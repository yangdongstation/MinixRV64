/* MinixRV64 Donz Build - Fork Implementation
 *
 * Process creation with Copy-On-Write support
 * Following HowToFitPosix.md Stage 2 design
 */

#include <minix/config.h>
#include <minix/task.h>
#include <minix/mm.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* External functions */
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);
extern unsigned long alloc_pages(int order);
extern void free_pages(unsigned long addr, int order);
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);

/* ============================================
 * Global State
 * ============================================ */

/* Process table - array of pointers */
struct task_struct *task_table[MAX_PROCS];

/* All tasks list */
LIST_HEAD(all_tasks);

/* Init task - statically initialized idle process (PID 0) */
struct task_struct init_task = {
    .state          = TASK_RUNNING,
    .flags          = PF_KTHREAD | PF_IDLE,
    .on_rq          = 0,
    .prio           = MAX_PRIO - 1,
    .static_prio    = MAX_PRIO - 1,
    .normal_prio    = MAX_PRIO - 1,
    .rt_priority    = 0,
    .policy         = SCHED_NORMAL,
    .time_slice     = DEF_TIMESLICE,
    .pid            = 0,
    .tgid           = 0,
    .ppid           = 0,
    .uid            = 0,
    .euid           = 0,
    .gid            = 0,
    .egid           = 0,
    .mm             = NULL,         /* Kernel thread has no user space */
    .active_mm      = NULL,
    .stack          = NULL,         /* Will be set later */
    .parent         = NULL,
    .real_parent    = NULL,
    .pgrp           = 0,
    .session        = 0,
    .group_leader   = NULL,
    .exit_code      = 0,
    .exit_signal    = 0,
    .comm           = "swapper",
    .tasks          = LIST_HEAD_INIT(init_task.tasks),
    .children       = LIST_HEAD_INIT(init_task.children),
    .sibling        = LIST_HEAD_INIT(init_task.sibling),
    .run_list       = LIST_HEAD_INIT(init_task.run_list),
};

/* ============================================
 * PID Allocation (Bitmap)
 * ============================================ */
#define PID_BITMAP_SIZE ((PID_MAX + 63) / 64)
static unsigned long pid_bitmap[PID_BITMAP_SIZE];
static spinlock_t pid_lock = SPIN_LOCK_INIT;
static pid_t last_pid = 0;

/* Find first zero bit in a word */
static inline int ffs_zero(unsigned long word)
{
    int bit = 0;
    if (word == ~0UL) return -1;
    while (word & (1UL << bit)) bit++;
    return bit;
}

/* Allocate a new PID */
pid_t alloc_pid(void)
{
    pid_t pid;
    int i, bit;

    spin_lock(&pid_lock);

    /* Start searching from last_pid + 1 */
    pid = last_pid + 1;
    if (pid >= PID_MAX) pid = 1;

    /* Search for free PID */
    for (i = 0; i < PID_BITMAP_SIZE; i++) {
        int idx = (pid / 64 + i) % PID_BITMAP_SIZE;
        if (pid_bitmap[idx] != ~0UL) {
            /* Found a word with free bit */
            bit = ffs_zero(pid_bitmap[idx]);
            if (bit >= 0) {
                pid = idx * 64 + bit;
                if (pid > 0 && pid < PID_MAX) {
                    pid_bitmap[idx] |= (1UL << bit);
                    last_pid = pid;
                    spin_unlock(&pid_lock);
                    return pid;
                }
            }
        }
    }

    spin_unlock(&pid_lock);
    return -1;  /* No free PID */
}

/* Free a PID */
void free_pid(pid_t pid)
{
    if (pid <= 0 || pid >= PID_MAX) return;

    spin_lock(&pid_lock);
    pid_bitmap[pid / 64] &= ~(1UL << (pid % 64));
    spin_unlock(&pid_lock);
}

/* Find task by PID */
struct task_struct *find_task_by_pid(pid_t pid)
{
    int i;
    for (i = 0; i < MAX_PROCS; i++) {
        if (task_table[i] && task_table[i]->pid == pid) {
            return task_table[i];
        }
    }
    return NULL;
}

/* ============================================
 * Task/Thread Info Allocation
 * ============================================ */

/* Allocate kernel stack + thread_info */
struct thread_info *alloc_thread_info(void)
{
    unsigned long page;
    struct thread_info *ti;

    /* Allocate 2 pages (8KB) for kernel stack */
    page = alloc_pages(THREAD_SIZE_ORDER);
    if (!page) {
        early_puts("[FORK] Failed to allocate kernel stack\n");
        return NULL;
    }

    /* thread_info is at the bottom of the stack */
    ti = (struct thread_info *)page;

    /* Zero out thread_info */
    ti->task = NULL;
    ti->flags = 0;
    ti->preempt_count = 0;
    ti->cpu = 0;
    ti->addr_limit = KERNEL_DS;
    ti->kernel_sp = page + THREAD_SIZE;
    ti->user_sp = 0;

    return ti;
}

/* Free kernel stack + thread_info */
void free_thread_info(struct thread_info *ti)
{
    if (ti) {
        free_pages((unsigned long)ti, THREAD_SIZE_ORDER);
    }
}

/* Setup thread_info for new task */
void setup_thread_info(struct thread_info *ti, struct task_struct *task)
{
    ti->task = task;
    ti->flags = 0;
    ti->preempt_count = 0;
    ti->cpu = 0;
    ti->addr_limit = KERNEL_DS;
}

/* Get thread_info from task */
static inline struct thread_info *task_thread_info_impl(struct task_struct *task)
{
    return (struct thread_info *)task->stack;
}

/* ============================================
 * Task Structure Allocation
 * ============================================ */

/* Simple task_struct cache (static array for now) */
#define TASK_CACHE_SIZE 64
static struct task_struct task_cache[TASK_CACHE_SIZE];
static unsigned long task_cache_bitmap = 0;

/* Allocate task_struct */
struct task_struct *alloc_task_struct(void)
{
    int i;

    /* Find free slot in cache */
    for (i = 0; i < TASK_CACHE_SIZE && i < 64; i++) {
        if (!(task_cache_bitmap & (1UL << i))) {
            task_cache_bitmap |= (1UL << i);
            /* Zero out the structure */
            struct task_struct *p = &task_cache[i];
            unsigned char *ptr = (unsigned char *)p;
            for (unsigned long j = 0; j < sizeof(struct task_struct); j++) {
                ptr[j] = 0;
            }
            return p;
        }
    }

    early_puts("[FORK] Task cache exhausted\n");
    return NULL;
}

/* Free task_struct */
void free_task_struct(struct task_struct *tsk)
{
    if (!tsk) return;

    /* Check if it's from our cache */
    int idx = tsk - task_cache;
    if (idx >= 0 && idx < TASK_CACHE_SIZE && idx < 64) {
        task_cache_bitmap &= ~(1UL << idx);
    }
}

/* ============================================
 * Fork Initialization
 * ============================================ */

void fork_init(void)
{
    int i;

    /* Clear process table */
    for (i = 0; i < MAX_PROCS; i++) {
        task_table[i] = NULL;
    }

    /* Clear PID bitmap, reserve PID 0 for idle */
    for (i = 0; i < PID_BITMAP_SIZE; i++) {
        pid_bitmap[i] = 0;
    }
    pid_bitmap[0] |= 1;  /* Reserve PID 0 */

    /* Clear task cache */
    task_cache_bitmap = 0;

    /* Setup init_task */
    init_task.group_leader = &init_task;
    init_task.parent = &init_task;
    init_task.real_parent = &init_task;

    /* Add init_task to process table */
    task_table[0] = &init_task;

    /* Initialize children and sibling lists properly */
    INIT_LIST_HEAD(&init_task.children);
    INIT_LIST_HEAD(&init_task.sibling);
    INIT_LIST_HEAD(&init_task.run_list);

    early_puts("✓ Fork subsystem\n");
}

/* ============================================
 * Memory Space Copying
 * ============================================ */

/* Allocate new mm_struct */
struct mm_struct *mm_alloc(void)
{
    struct mm_struct *mm;

    mm = (struct mm_struct *)kmalloc(sizeof(struct mm_struct));
    if (!mm) return NULL;

    /* Zero initialize */
    unsigned char *ptr = (unsigned char *)mm;
    for (unsigned long i = 0; i < sizeof(struct mm_struct); i++) {
        ptr[i] = 0;
    }

    atomic_set(&mm->mm_users, 1);
    atomic_set(&mm->mm_count, 1);
    spin_lock_init(&mm->page_table_lock);

    return mm;
}

/* Free mm_struct */
void mm_free(struct mm_struct *mm)
{
    if (mm) {
        kfree(mm);
    }
}

/* Copy memory space (simplified - no COW yet) */
int copy_mm(unsigned long clone_flags, struct task_struct *p)
{
    struct mm_struct *mm, *oldmm;
    struct task_struct *current_task = get_current();

    if (!current_task) {
        p->mm = NULL;
        p->active_mm = NULL;
        return 0;
    }

    oldmm = current_task->mm;

    /* Kernel threads have no user space */
    if (!oldmm) {
        p->mm = NULL;
        p->active_mm = NULL;
        return 0;
    }

    /* CLONE_VM: Share address space (threads) */
    if (clone_flags & CLONE_VM) {
        atomic_inc(&oldmm->mm_users);
        p->mm = oldmm;
        p->active_mm = oldmm;
        return 0;
    }

    /* Allocate new mm */
    mm = mm_alloc();
    if (!mm) return -1;

    /* Copy mm contents (shallow copy for now) */
    mm->start_code = oldmm->start_code;
    mm->end_code = oldmm->end_code;
    mm->start_data = oldmm->start_data;
    mm->end_data = oldmm->end_data;
    mm->start_brk = oldmm->start_brk;
    mm->brk = oldmm->brk;
    mm->start_stack = oldmm->start_stack;
    mm->arg_start = oldmm->arg_start;
    mm->arg_end = oldmm->arg_end;
    mm->env_start = oldmm->env_start;
    mm->env_end = oldmm->env_end;

    /* TODO: Allocate new page table and implement COW */
    /* For now, we don't copy page tables */
    mm->pgd = NULL;
    mm->mmap = NULL;
    mm->map_count = 0;

    p->mm = mm;
    p->active_mm = mm;

    return 0;
}

/* ============================================
 * File Descriptor Copying
 * ============================================ */

int copy_files(unsigned long clone_flags, struct task_struct *p)
{
    struct task_struct *current_task = get_current();
    int i;

    if (!current_task) {
        for (i = 0; i < MAX_OPEN_FILES; i++) {
            p->ofile[i] = NULL;
        }
        return 0;
    }

    /* CLONE_FILES: Share file descriptors */
    if (clone_flags & CLONE_FILES) {
        /* Share the files_struct (if we had one) */
        p->files = current_task->files;
        if (p->files) {
            atomic_inc(&p->files->count);
        }
    } else {
        /* Copy file descriptors */
        for (i = 0; i < MAX_OPEN_FILES; i++) {
            p->ofile[i] = current_task->ofile[i];
            /* TODO: Increment reference counts */
        }
    }

    return 0;
}

/* ============================================
 * Filesystem Info Copying
 * ============================================ */

int copy_fs(unsigned long clone_flags, struct task_struct *p)
{
    struct task_struct *current_task = get_current();

    if (!current_task) {
        p->fs = NULL;
        p->cwd = NULL;
        return 0;
    }

    /* CLONE_FS: Share filesystem info */
    if (clone_flags & CLONE_FS) {
        p->fs = current_task->fs;
        if (p->fs) {
            atomic_inc(&p->fs->count);
        }
    } else {
        /* Copy fs_struct (simplified) */
        p->fs = NULL;  /* TODO: Implement fs_struct allocation */
    }

    p->cwd = current_task->cwd;

    return 0;
}

/* ============================================
 * Signal Handler Copying
 * ============================================ */

int copy_sighand(unsigned long clone_flags, struct task_struct *p)
{
    struct task_struct *current_task = get_current();

    (void)clone_flags;  /* TODO: Handle CLONE_SIGHAND */

    /* Initialize signal state */
    p->blocked = 0;
    INIT_LIST_HEAD(&p->pending.list);
    p->pending.signal = 0;

    if (!current_task) {
        return 0;
    }

    /* Copy blocked signals */
    p->blocked = current_task->blocked;

    return 0;
}

/* ============================================
 * Thread/CPU Context Copying
 * ============================================ */

int copy_thread(unsigned long clone_flags, unsigned long usp,
                struct trapframe *regs, struct task_struct *p)
{
    struct trapframe *childregs;
    struct thread_struct *thread = &p->thread;

    (void)clone_flags;  /* TODO: Handle thread flags */

    /* Get child's pt_regs at top of kernel stack */
    childregs = task_pt_regs(p);
    p->trapframe = childregs;

    /* Copy parent's trap frame if available */
    if (regs) {
        /* Copy all registers */
        unsigned char *dst = (unsigned char *)childregs;
        unsigned char *src = (unsigned char *)regs;
        for (unsigned long i = 0; i < sizeof(struct trapframe); i++) {
            dst[i] = src[i];
        }

        /* Child returns 0 from fork */
        childregs->a0 = 0;

        /* Use specified stack if provided */
        if (usp) {
            childregs->sp = usp;
        }
    } else {
        /* Kernel thread - clear trap frame */
        unsigned char *ptr = (unsigned char *)childregs;
        for (unsigned long i = 0; i < sizeof(struct trapframe); i++) {
            ptr[i] = 0;
        }
    }

    /* Setup kernel thread entry point */
    thread->ra = (unsigned long)ret_from_fork;
    thread->sp = (unsigned long)childregs;

    /* Clear callee-saved registers */
    for (int i = 0; i < 12; i++) {
        thread->s[i] = 0;
    }

    /* Setup context for swtch() */
    p->context.ra = (unsigned long)ret_from_fork;
    p->context.sp = (unsigned long)childregs;
    p->context.s0 = 0;

    return 0;
}

/* ============================================
 * Main Fork Implementation
 * ============================================ */

/* Copy process - main fork logic */
struct task_struct *copy_process(unsigned long clone_flags,
                                  unsigned long stack_start,
                                  struct trapframe *regs)
{
    struct task_struct *p;
    struct thread_info *ti;
    struct task_struct *current_task = get_current();
    int retval;
    int slot;

    /* 1. Allocate task_struct */
    p = alloc_task_struct();
    if (!p) {
        early_puts("[FORK] Failed to allocate task_struct\n");
        return ERR_PTR(-1);
    }

    /* 2. Allocate kernel stack + thread_info */
    ti = alloc_thread_info();
    if (!ti) {
        free_task_struct(p);
        early_puts("[FORK] Failed to allocate thread_info\n");
        return ERR_PTR(-1);
    }

    /* 3. Copy parent's basic info */
    if (current_task) {
        p->state = TASK_RUNNING;
        p->flags = current_task->flags & ~PF_IDLE;
        p->prio = current_task->prio;
        p->static_prio = current_task->static_prio;
        p->normal_prio = current_task->normal_prio;
        p->policy = current_task->policy;
        p->uid = current_task->uid;
        p->euid = current_task->euid;
        p->gid = current_task->gid;
        p->egid = current_task->egid;
    } else {
        p->state = TASK_RUNNING;
        p->flags = 0;
        p->prio = DEFAULT_PRIO;
        p->static_prio = DEFAULT_PRIO;
        p->normal_prio = DEFAULT_PRIO;
        p->policy = SCHED_NORMAL;
    }

    /* Setup thread_info */
    ti->task = p;
    p->stack = ti;

    /* 4. Allocate PID */
    p->pid = alloc_pid();
    if (p->pid < 0) {
        free_thread_info(ti);
        free_task_struct(p);
        early_puts("[FORK] Failed to allocate PID\n");
        return ERR_PTR(-1);
    }

    /* Set thread group ID */
    if (clone_flags & CLONE_THREAD) {
        p->tgid = current_task ? current_task->tgid : p->pid;
    } else {
        p->tgid = p->pid;
    }

    /* Set parent PID */
    p->ppid = current_task ? current_task->pid : 0;

    /* 5. Initialize scheduling state */
    p->on_rq = 0;
    p->time_slice = DEF_TIMESLICE;
    p->utime = 0;
    p->stime = 0;
    p->nvcsw = 0;
    p->nivcsw = 0;

    /* 6. Copy memory space */
    retval = copy_mm(clone_flags, p);
    if (retval) goto bad_fork_cleanup;

    /* 7. Copy file descriptors */
    retval = copy_files(clone_flags, p);
    if (retval) goto bad_fork_cleanup_mm;

    /* 8. Copy filesystem info */
    retval = copy_fs(clone_flags, p);
    if (retval) goto bad_fork_cleanup_files;

    /* 9. Copy signal handling */
    retval = copy_sighand(clone_flags, p);
    if (retval) goto bad_fork_cleanup_fs;

    /* 10. Copy CPU context */
    retval = copy_thread(clone_flags, stack_start, regs, p);
    if (retval) goto bad_fork_cleanup_sighand;

    /* 11. Initialize lists */
    INIT_LIST_HEAD(&p->children);
    INIT_LIST_HEAD(&p->sibling);
    INIT_LIST_HEAD(&p->run_list);

    /* 12. Setup parent-child relationship */
    p->parent = current_task ? current_task : &init_task;
    p->real_parent = p->parent;
    p->group_leader = (clone_flags & CLONE_THREAD) ?
                      (current_task ? current_task->group_leader : p) : p;

    /* Add to parent's children list */
    if (p->parent != p) {
        list_add_tail(&p->sibling, &p->parent->children);
    }

    /* 13. Copy process name */
    if (current_task) {
        for (int i = 0; i < TASK_COMM_LEN; i++) {
            p->comm[i] = current_task->comm[i];
        }
    } else {
        p->comm[0] = 'f';
        p->comm[1] = 'o';
        p->comm[2] = 'r';
        p->comm[3] = 'k';
        p->comm[4] = '\0';
    }

    /* 14. Initialize exit state */
    p->exit_code = 0;
    p->exit_signal = (clone_flags & CLONE_THREAD) ? -1 : SIGCHLD;
    p->killed = 0;
    p->chan = NULL;
    init_waitqueue_head(&p->wait_chldexit);

    /* 15. Add to process table */
    for (slot = 0; slot < MAX_PROCS; slot++) {
        if (task_table[slot] == NULL) {
            task_table[slot] = p;
            break;
        }
    }

    /* 16. Add to all tasks list */
    list_add_tail(&p->tasks, &init_task.tasks);

    return p;

bad_fork_cleanup_sighand:
    /* TODO: exit_sighand(p); */
bad_fork_cleanup_fs:
    /* TODO: exit_fs(p); */
bad_fork_cleanup_files:
    /* TODO: exit_files(p); */
bad_fork_cleanup_mm:
    /* TODO: exit_mm(p); */
bad_fork_cleanup:
    free_pid(p->pid);
    free_thread_info(ti);
    free_task_struct(p);
    return ERR_PTR(retval);
}

/* do_fork - main fork entry point */
pid_t do_fork(unsigned long clone_flags, unsigned long stack_start,
              struct trapframe *regs)
{
    struct task_struct *p;

    /* Create new process */
    p = copy_process(clone_flags, stack_start, regs);
    if (IS_ERR(p)) {
        return PTR_ERR(p);
    }

    /* Wake up the new task */
    wake_up_new_task(p);

    /* For vfork, wait for child to exec or exit */
    if (clone_flags & CLONE_VFORK) {
        /* TODO: Implement vfork completion waiting */
    }

    return p->pid;
}

/* ============================================
 * System Call Wrappers
 * ============================================ */

/* fork() system call */
pid_t sys_fork(void)
{
    struct trapframe *regs = current_pt_regs();
    return do_fork(SIGCHLD, 0, regs);
}

/* vfork() system call */
pid_t sys_vfork(void)
{
    struct trapframe *regs = current_pt_regs();
    return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, 0, regs);
}

/* clone() system call */
pid_t sys_clone(unsigned long clone_flags, unsigned long newsp,
                void *parent_tidptr, unsigned long tls, void *child_tidptr)
{
    struct trapframe *regs = current_pt_regs();
    (void)parent_tidptr;
    (void)tls;
    (void)child_tidptr;
    return do_fork(clone_flags, newsp, regs);
}

/* ============================================
 * Current Task Management
 * ============================================ */

static struct task_struct *current_task_ptr = NULL;

struct task_struct *get_current(void)
{
    /* Try to get from thread_info first */
    /* For now, use global pointer */
    return current_task_ptr ? current_task_ptr : &init_task;
}

void set_current(struct task_struct *p)
{
    current_task_ptr = p;
}

/* ============================================
 * Kernel Thread Creation
 * ============================================ */

/* Entry point for kernel threads */
static void kernel_thread_helper(void)
{
    struct task_struct *tsk = get_current();
    int (*fn)(void *);
    void *arg;

    if (!tsk) {
        early_puts("[KTHREAD] No current task!\n");
        do_exit(-1);
        return;
    }

    /* Get function and argument from thread struct */
    fn = (int (*)(void *))tsk->thread.kthread_fn;
    arg = (void *)tsk->thread.kthread_arg;

    if (fn) {
        int ret = fn(arg);
        do_exit(ret);
    } else {
        do_exit(-1);
    }
}

/* Create a kernel thread */
pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
    struct task_struct *p;
    struct thread_info *ti;
    int slot;

    (void)flags;  /* TODO: Use clone flags */

    /* 1. Allocate task_struct */
    p = alloc_task_struct();
    if (!p) {
        early_puts("[KTHREAD] Failed to allocate task_struct\n");
        return -1;
    }

    /* 2. Allocate kernel stack + thread_info */
    ti = alloc_thread_info();
    if (!ti) {
        free_task_struct(p);
        early_puts("[KTHREAD] Failed to allocate thread_info\n");
        return -1;
    }

    /* 3. Initialize task */
    p->state = TASK_RUNNING;
    p->flags = PF_KTHREAD;
    p->prio = DEFAULT_PRIO;
    p->static_prio = DEFAULT_PRIO;
    p->normal_prio = DEFAULT_PRIO;
    p->policy = SCHED_NORMAL;
    p->uid = 0;
    p->euid = 0;
    p->gid = 0;
    p->egid = 0;

    /* Setup thread_info */
    ti->task = p;
    p->stack = ti;

    /* 4. Allocate PID */
    p->pid = alloc_pid();
    if (p->pid < 0) {
        free_thread_info(ti);
        free_task_struct(p);
        early_puts("[KTHREAD] Failed to allocate PID\\n");
        return -1;
    }
    p->tgid = p->pid;
    p->ppid = 0;

    /* 5. Kernel thread has no user address space */
    p->mm = NULL;
    p->active_mm = NULL;

    /* 6. Initialize scheduling state */
    p->on_rq = 0;
    p->time_slice = DEF_TIMESLICE;
    p->utime = 0;
    p->stime = 0;

    /* 7. Store function and argument */
    p->thread.kthread_fn = (unsigned long)fn;
    p->thread.kthread_arg = (unsigned long)arg;

    /* 8. Setup kernel thread entry point */
    /* When switched to, will run kernel_thread_helper */
    p->thread.ra = (unsigned long)kernel_thread_helper;
    p->thread.sp = (unsigned long)ti + THREAD_SIZE;

    /* Setup context for swtch() */
    p->context.ra = (unsigned long)kernel_thread_helper;
    p->context.sp = (unsigned long)ti + THREAD_SIZE - 16; /* Leave some space */
    p->context.s0 = 0;

    /* 9. Initialize lists */
    INIT_LIST_HEAD(&p->children);
    INIT_LIST_HEAD(&p->sibling);
    INIT_LIST_HEAD(&p->run_list);

    /* 10. Setup parent-child relationship */
    p->parent = &init_task;
    p->real_parent = &init_task;
    p->group_leader = p;
    list_add_tail(&p->sibling, &init_task.children);

    /* 11. Set name */
    p->comm[0] = 'k';
    p->comm[1] = 't';
    p->comm[2] = 'h';
    p->comm[3] = 'r';
    p->comm[4] = 'e';
    p->comm[5] = 'a';
    p->comm[6] = 'd';
    p->comm[7] = '\0';

    /* 12. Initialize exit state */
    p->exit_code = 0;
    p->exit_signal = 0;
    p->killed = 0;
    p->chan = NULL;
    init_waitqueue_head(&p->wait_chldexit);

    /* 13. Add to process table */
    for (slot = 0; slot < MAX_PROCS; slot++) {
        if (task_table[slot] == NULL) {
            task_table[slot] = p;
            break;
        }
    }

    /* 14. Add to all tasks list */
    list_add_tail(&p->tasks, &init_task.tasks);

    /* 15. Wake up the new kernel thread */
    wake_up_new_task(p);

    early_puts("[KTHREAD] Created kernel thread PID ");
    early_puthex(p->pid);
    early_puts("\n");

    return p->pid;
}
