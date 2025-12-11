/* MinixRV64 Donz Build - Scheduler Header
 *
 * Enhanced scheduler with O(1) priority support
 * Following HowToFitPosix.md Stage 2 design
 */

#ifndef _MINIX_SCHED_H
#define _MINIX_SCHED_H

#include <types.h>

/* ============================================
 * Process States
 * ============================================ */
#define TASK_RUNNING        0   /* Runnable (ready or running) */
#define TASK_INTERRUPTIBLE  1   /* Interruptible sleep */
#define TASK_UNINTERRUPTIBLE 2  /* Uninterruptible sleep */
#define TASK_ZOMBIE         4   /* Zombie process */
#define TASK_STOPPED        8   /* Stopped */
#define TASK_DEAD           16  /* Dead, being removed */

/* ============================================
 * Process Flags
 * ============================================ */
#define PF_KTHREAD          0x00000001  /* Kernel thread */
#define PF_EXITING          0x00000004  /* Exiting process */
#define PF_FORKNOEXEC       0x00000040  /* Forked but not exec'd */
#define PF_IDLE             0x00000100  /* Idle process */

/* ============================================
 * Scheduling Policies
 * ============================================ */
#define SCHED_NORMAL        0   /* Normal time-sharing */
#define SCHED_FIFO          1   /* Real-time FIFO */
#define SCHED_RR            2   /* Real-time Round-Robin */

/* ============================================
 * Priority Definitions
 * ============================================ */
#define MAX_PRIO            140
#define DEFAULT_PRIO        120
#define MAX_USER_PRIO       100
#define MAX_RT_PRIO         100
#define MIN_NICE            (-20)
#define MAX_NICE            19
#define DEFAULT_NICE        0

/* ============================================
 * Time Slice Definitions (in ticks)
 * ============================================ */
#define DEF_TIMESLICE       10  /* Default 10 ticks */
#define MIN_TIMESLICE       1
#define MAX_TIMESLICE       100

/* ============================================
 * Clone Flags (for fork/clone)
 * ============================================ */
#define CLONE_VM            0x00000100  /* Share memory space */
#define CLONE_FS            0x00000200  /* Share filesystem info */
#define CLONE_FILES         0x00000400  /* Share file descriptors */
#define CLONE_SIGHAND       0x00000800  /* Share signal handlers */
#define CLONE_THREAD        0x00010000  /* Same thread group */
#define CLONE_VFORK         0x00004000  /* vfork semantics */
#define CLONE_PARENT        0x00008000  /* Same parent */

/* Signal for child termination */
#define SIGCHLD             17

/* ============================================
 * List Head Structure (must be defined first)
 * ============================================ */
struct list_head {
    struct list_head *next, *prev;
};

/* ============================================
 * List Operations (minimal implementation)
 * ============================================ */
#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = (void *)0;
    entry->prev = (void *)0;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_for_each_entry(pos, head, member)                  \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
         &pos->member != (head);                                \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = list_entry((head)->next, typeof(*pos), member),  \
         n = list_entry(pos->member.next, typeof(*pos), member);\
         &pos->member != (head);                                \
         pos = n, n = list_entry(n->member.next, typeof(*n), member))

/* ============================================
 * Scheduler Entity (for CFS-like scheduling)
 * ============================================ */
struct sched_entity {
    unsigned int on_rq;             /* On run queue */
    unsigned long vruntime;         /* Virtual runtime */
    unsigned long sum_exec_runtime; /* Total execution time */
    unsigned long prev_sum_exec_runtime;
};

/* ============================================
 * Priority Array (for O(1) scheduler)
 * ============================================ */
#define BITMAP_SIZE         ((MAX_PRIO + 63) / 64)

struct prio_array {
    unsigned int nr_active;                     /* Active task count */
    unsigned long bitmap[BITMAP_SIZE];          /* Priority bitmap */
    struct list_head queue[MAX_PRIO];           /* Queue per priority */
};

/* ============================================
 * Run Queue (per-CPU)
 * ============================================ */
struct rq {
    unsigned long lock;             /* Spinlock */
    unsigned long nr_running;       /* Number of runnable tasks */
    unsigned long nr_switches;      /* Context switch count */

    struct task_struct *curr;       /* Currently running task */
    struct task_struct *idle;       /* Idle task */

    /* O(1) scheduler arrays */
    struct prio_array *active;      /* Active priority array */
    struct prio_array *expired;     /* Expired priority array */
    struct prio_array arrays[2];    /* The two arrays */

    /* Timing */
    unsigned long clock;            /* Run queue clock */
    unsigned long clock_task;       /* Task clock */
};

/* ============================================
 * Function Declarations
 * ============================================ */

/* Scheduler initialization */
void sched_init(void);

/* Schedule next task */
void schedule(void);

/* Yield current CPU time */
void yield(void);

/* Timer tick handling */
void scheduler_tick(void);

/* Wake up a process */
int wake_up_process(struct task_struct *p);

/* Wake up a new task */
void wake_up_new_task(struct task_struct *p);

/* Context switch */
void context_switch(struct rq *rq, struct task_struct *prev, struct task_struct *next);

/* Get current run queue */
struct rq *this_rq(void);

/* Check if rescheduling is needed */
int need_resched(void);

/* Set need_resched flag */
void set_tsk_need_resched(struct task_struct *tsk);

/* Clear need_resched flag */
void clear_tsk_need_resched(struct task_struct *tsk);

/* Calculate timeslice for a task */
unsigned long task_timeslice(struct task_struct *p);

/* CPU idle loop */
void cpu_idle(void);

/* Sleep/Wakeup */
void sleep(void *chan);
void wakeup(void *chan);

#endif /* _MINIX_SCHED_H */
