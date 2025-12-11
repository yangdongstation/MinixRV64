/* MinixRV64 Donz Build - O(1) Priority Scheduler
 *
 * Simplified O(1) scheduler with active/expired arrays
 * Following HowToFitPosix.md Stage 2 design
 */

#include <minix/config.h>
#include <minix/task.h>
#include <minix/sched.h>
#include <minix/mm.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* External functions */
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);
extern void swtch(struct context *old, struct context *new);

/* ============================================
 * Global Run Queue (Single CPU)
 * ============================================ */
static struct rq runqueue;

/* ============================================
 * Bitmap Operations
 * ============================================ */

/* Find first set bit */
static inline int find_first_bit(unsigned long *bitmap, int size)
{
    int i, bit;
    int words = (size + 63) / 64;

    for (i = 0; i < words; i++) {
        if (bitmap[i]) {
            for (bit = 0; bit < 64; bit++) {
                if (bitmap[i] & (1UL << bit)) {
                    int result = i * 64 + bit;
                    return result < size ? result : size;
                }
            }
        }
    }
    return size;
}

static inline void __set_bit(int nr, unsigned long *addr)
{
    addr[nr / 64] |= (1UL << (nr % 64));
}

static inline void __clear_bit(int nr, unsigned long *addr)
{
    addr[nr / 64] &= ~(1UL << (nr % 64));
}

static inline int test_bit(int nr, unsigned long *addr)
{
    return (addr[nr / 64] & (1UL << (nr % 64))) != 0;
}

/* ============================================
 * Scheduler Initialization
 * ============================================ */

void sched_init(void)
{
    struct rq *rq = &runqueue;
    int i;

    /* Initialize spinlock */
    spin_lock_init((spinlock_t *)&rq->lock);

    rq->nr_running = 0;
    rq->nr_switches = 0;
    rq->clock = 0;
    rq->clock_task = 0;

    /* Initialize priority arrays */
    for (i = 0; i < 2; i++) {
        struct prio_array *array = &rq->arrays[i];
        int j;

        array->nr_active = 0;

        /* Clear bitmap */
        for (j = 0; j < BITMAP_SIZE; j++) {
            array->bitmap[j] = 0;
        }

        /* Initialize all priority queues */
        for (j = 0; j < MAX_PRIO; j++) {
            INIT_LIST_HEAD(&array->queue[j]);
        }
    }

    rq->active = &rq->arrays[0];
    rq->expired = &rq->arrays[1];

    /* Set idle task as init_task */
    rq->idle = &init_task;
    rq->curr = &init_task;

    early_puts("✓ O(1) Scheduler\n");
}

/* ============================================
 * Get Current Run Queue
 * ============================================ */

struct rq *this_rq(void)
{
    return &runqueue;
}

/* ============================================
 * Enqueue/Dequeue Tasks
 * ============================================ */

/* Add task to run queue */
static void enqueue_task(struct rq *rq, struct task_struct *p)
{
    struct prio_array *array = rq->active;
    int prio = p->prio;

    if (prio < 0) prio = 0;
    if (prio >= MAX_PRIO) prio = MAX_PRIO - 1;

    list_add_tail(&p->run_list, &array->queue[prio]);
    __set_bit(prio, array->bitmap);
    array->nr_active++;
    rq->nr_running++;
    p->on_rq = 1;
}

/* Remove task from run queue */
static void dequeue_task(struct rq *rq, struct task_struct *p)
{
    struct prio_array *array;
    int prio = p->prio;

    if (!p->on_rq) return;

    if (prio < 0) prio = 0;
    if (prio >= MAX_PRIO) prio = MAX_PRIO - 1;

    /* Try active array first */
    array = rq->active;
    list_del(&p->run_list);
    INIT_LIST_HEAD(&p->run_list);

    if (list_empty(&array->queue[prio])) {
        __clear_bit(prio, array->bitmap);
    }

    if (array->nr_active > 0) array->nr_active--;
    if (rq->nr_running > 0) rq->nr_running--;
    p->on_rq = 0;
}

/* ============================================
 * Task Selection
 * ============================================ */

/* Pick next task to run */
static struct task_struct *pick_next_task(struct rq *rq)
{
    struct prio_array *array = rq->active;
    struct task_struct *next;
    int idx;

    /* If active queue is empty, swap arrays */
    if (array->nr_active == 0) {
        struct prio_array *tmp = rq->active;
        rq->active = rq->expired;
        rq->expired = tmp;
        array = rq->active;
    }

    /* Still empty? Return idle */
    if (array->nr_active == 0) {
        return rq->idle;
    }

    /* Find highest priority (lowest number) with tasks */
    idx = find_first_bit(array->bitmap, MAX_PRIO);
    if (idx >= MAX_PRIO) {
        return rq->idle;
    }

    /* Get first task from queue */
    if (list_empty(&array->queue[idx])) {
        return rq->idle;
    }

    next = list_first_entry(&array->queue[idx], struct task_struct, run_list);
    return next;
}

/* ============================================
 * Time Slice Calculation
 * ============================================ */

unsigned long task_timeslice(struct task_struct *p)
{
    unsigned long timeslice;

    /* Higher priority (lower number) = longer timeslice */
    if (p->prio < 100) {
        /* Real-time priority: fixed timeslice */
        timeslice = MAX_TIMESLICE;
    } else {
        /* Normal priority: scale by priority */
        int nice = p->prio - DEFAULT_PRIO;
        timeslice = DEF_TIMESLICE - nice;
        if (timeslice < MIN_TIMESLICE) timeslice = MIN_TIMESLICE;
        if (timeslice > MAX_TIMESLICE) timeslice = MAX_TIMESLICE;
    }

    return timeslice;
}

/* ============================================
 * Timer Tick Handler
 * ============================================ */

void scheduler_tick(void)
{
    struct rq *rq = &runqueue;
    struct task_struct *curr = rq->curr;

    /* Lock run queue */
    spin_lock((spinlock_t *)&rq->lock);

    /* Update clock */
    rq->clock++;

    /* Update process time accounting */
    if (curr && curr != rq->idle) {
        /* For now, always count as system time */
        curr->stime++;

        /* Decrement time slice */
        if (curr->time_slice > 0) {
            curr->time_slice--;
        }

        /* Time slice exhausted */
        if (curr->time_slice == 0) {
            /* Recalculate time slice */
            curr->time_slice = task_timeslice(curr);

            /* Move to expired queue */
            if (curr->on_rq) {
                dequeue_task(rq, curr);

                /* Add to expired queue */
                struct prio_array *array = rq->expired;
                int prio = curr->prio;
                if (prio < 0) prio = 0;
                if (prio >= MAX_PRIO) prio = MAX_PRIO - 1;

                list_add_tail(&curr->run_list, &array->queue[prio]);
                __set_bit(prio, array->bitmap);
                array->nr_active++;
                rq->nr_running++;
                curr->on_rq = 1;
            }

            /* Mark need reschedule */
            set_tsk_need_resched(curr);
        }
    }

    spin_unlock((spinlock_t *)&rq->lock);
}

/* ============================================
 * Need Resched Flag
 * ============================================ */

int need_resched(void)
{
    struct task_struct *curr = get_current();
    if (!curr || !curr->stack) return 0;

    struct thread_info *ti = (struct thread_info *)curr->stack;
    return test_ti_thread_flag(ti, TIF_NEED_RESCHED);
}

void set_tsk_need_resched(struct task_struct *tsk)
{
    if (tsk && tsk->stack) {
        struct thread_info *ti = (struct thread_info *)tsk->stack;
        set_ti_thread_flag(ti, TIF_NEED_RESCHED);
    }
}

void clear_tsk_need_resched(struct task_struct *tsk)
{
    if (tsk && tsk->stack) {
        struct thread_info *ti = (struct thread_info *)tsk->stack;
        clear_ti_thread_flag(ti, TIF_NEED_RESCHED);
    }
}

/* ============================================
 * Context Switch
 * ============================================ */

/* Switch address space */
void switch_mm(struct mm_struct *prev_mm, struct mm_struct *next_mm,
               struct task_struct *next)
{
    (void)prev_mm;
    (void)next;

    if (next_mm && next_mm->pgd) {
        /* Load new page table */
        unsigned long satp = (8UL << 60) |
                             ((unsigned long)next_mm->pgd >> 12);
        asm volatile ("csrw satp, %0" :: "r"(satp));
        asm volatile ("sfence.vma" ::: "memory");
    }
}

/* Full context switch */
void context_switch(struct rq *rq, struct task_struct *prev,
                    struct task_struct *next)
{
    struct mm_struct *mm = next->mm;
    struct mm_struct *oldmm = prev->active_mm;

    (void)rq;  /* Single CPU, rq not used */

    /* Switch address space */
    if (!mm) {
        /* Kernel thread - borrow previous mm */
        next->active_mm = oldmm;
        if (oldmm) {
            atomic_inc(&oldmm->mm_count);
        }
    } else {
        switch_mm(oldmm, mm, next);
    }

    /* Release previous mm if kernel thread */
    if (!prev->mm) {
        prev->active_mm = NULL;
        if (oldmm) {
            atomic_dec(&oldmm->mm_count);
        }
    }

    /* Switch registers */
    swtch(&prev->context, &next->context);
}

/* ============================================
 * Main Scheduler Function
 * ============================================ */

void schedule(void)
{
    struct rq *rq = &runqueue;
    struct task_struct *prev, *next;

    /* Disable interrupts and lock */
    /* TODO: local_irq_save(flags); */
    spin_lock((spinlock_t *)&rq->lock);

    prev = rq->curr;

    /* Remove current from queue if not runnable */
    if (prev && prev->state != TASK_RUNNING && prev->on_rq) {
        dequeue_task(rq, prev);
    }

    /* Pick next task */
    next = pick_next_task(rq);

    /* Clear resched flag */
    if (prev) {
        clear_tsk_need_resched(prev);
    }

    if (prev != next) {
        rq->nr_switches++;
        rq->curr = next;

        /* Update current pointer */
        set_current(next);

        /* Context switch */
        spin_unlock((spinlock_t *)&rq->lock);
        context_switch(rq, prev, next);
        /* After switch, we're in new task context */
        spin_lock((spinlock_t *)&rq->lock);
    }

    spin_unlock((spinlock_t *)&rq->lock);
    /* TODO: local_irq_restore(flags); */
}

/* ============================================
 * Yield
 * ============================================ */

void yield(void)
{
    struct task_struct *curr = get_current();

    if (curr && curr->state == TASK_RUNNING) {
        /* Move to end of run queue */
        struct rq *rq = &runqueue;
        spin_lock((spinlock_t *)&rq->lock);

        if (curr->on_rq) {
            dequeue_task(rq, curr);
            enqueue_task(rq, curr);
        }

        spin_unlock((spinlock_t *)&rq->lock);
    }

    schedule();
}

/* ============================================
 * Wake Up Functions
 * ============================================ */

int wake_up_process(struct task_struct *p)
{
    struct rq *rq = &runqueue;

    if (!p) return 0;

    spin_lock((spinlock_t *)&rq->lock);

    if (p->state == TASK_RUNNING) {
        spin_unlock((spinlock_t *)&rq->lock);
        return 0;
    }

    p->state = TASK_RUNNING;

    if (!p->on_rq) {
        enqueue_task(rq, p);
    }

    /* Preempt current if higher priority */
    if (rq->curr && p->prio < rq->curr->prio) {
        set_tsk_need_resched(rq->curr);
    }

    spin_unlock((spinlock_t *)&rq->lock);
    return 1;
}

void wake_up_new_task(struct task_struct *p)
{
    struct rq *rq = &runqueue;

    if (!p) return;

    spin_lock((spinlock_t *)&rq->lock);

    p->state = TASK_RUNNING;
    p->time_slice = task_timeslice(p);

    enqueue_task(rq, p);

    spin_unlock((spinlock_t *)&rq->lock);
}

/* ============================================
 * Sleep/Wakeup (Legacy compatibility)
 * ============================================ */

void sleep(void *chan)
{
    struct task_struct *p = get_current();

    if (!p) return;

    p->chan = chan;
    p->state = TASK_INTERRUPTIBLE;

    schedule();
}

void wakeup(void *chan)
{
    int i;
    struct task_struct *p;

    for (i = 0; i < MAX_PROCS; i++) {
        p = task_table[i];
        if (p && (p->state == TASK_INTERRUPTIBLE ||
                  p->state == TASK_UNINTERRUPTIBLE) &&
            p->chan == chan) {
            p->chan = NULL;
            wake_up_process(p);
        }
    }
}

/* ============================================
 * Scheduler Tail (called after fork switch)
 * ============================================ */

void schedule_tail(void)
{
    /* Called by ret_from_fork after context switch completes */
    /* Finish any pending scheduler work */
    struct rq *rq = &runqueue;
    (void)rq;

    /* For now, nothing special needed */
}

/* ============================================
 * CPU Idle
 * ============================================ */

void cpu_idle(void)
{
    while (1) {
        while (!need_resched()) {
            /* Wait for interrupt */
            asm volatile ("wfi");
        }
        schedule();
    }
}
