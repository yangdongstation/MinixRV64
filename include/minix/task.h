/* MinixRV64 Donz Build - Enhanced Process Management
 *
 * Complete task_struct and process management
 * Following HowToFitPosix.md Stage 2 design
 */

#ifndef _MINIX_TASK_H
#define _MINIX_TASK_H

#include <types.h>
#include <minix/sched.h>
#include <minix/mm_types.h>
#include <minix/thread_info.h>

/* ============================================
 * Type Definitions
 * Note: pid_t, uid_t, gid_t, u64, u32 already defined in types.h
 * We use #ifndef to avoid conflicts
 * ============================================ */
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
/* pid_t already in types.h */
#endif

/* ============================================
 * Maximum Limits
 * ============================================ */
#define MAX_PROCS           64      /* Maximum processes */
#define MAX_OPEN_FILES      16      /* Files per process */
#define TASK_COMM_LEN       16      /* Process name length */
#define PID_MAX             32768   /* Maximum PID */

/* ============================================
 * Context Structure (for context switch)
 *
 * Only callee-saved registers are stored here.
 * This is used by swtch() for kernel context switch.
 * ============================================ */
struct context {
    unsigned long ra;       /* x1  - Return address */
    unsigned long sp;       /* x2  - Stack pointer */
    unsigned long s0;       /* x8  - Frame pointer */
    unsigned long s1;       /* x9  */
    unsigned long s2;       /* x18 */
    unsigned long s3;       /* x19 */
    unsigned long s4;       /* x20 */
    unsigned long s5;       /* x21 */
    unsigned long s6;       /* x22 */
    unsigned long s7;       /* x23 */
    unsigned long s8;       /* x24 */
    unsigned long s9;       /* x25 */
    unsigned long s10;      /* x26 */
    unsigned long s11;      /* x27 */
};

/* Size of context in bytes */
#define CONTEXT_SIZE    (14 * 8)

/* ============================================
 * Thread Structure (arch-specific)
 *
 * Additional architecture-specific thread state
 * ============================================ */
struct thread_struct {
    unsigned long ra;           /* Return address */
    unsigned long sp;           /* Stack pointer */
    unsigned long s[12];        /* s0-s11 callee-saved registers */
    unsigned long sepc;         /* Exception PC */
    unsigned long sstatus;      /* Status register */
    unsigned long kernel_sp;    /* Kernel stack top */
    unsigned long user_sp;      /* User stack pointer */
    /* For kernel threads */
    unsigned long kthread_fn;   /* Kernel thread function */
    unsigned long kthread_arg;  /* Kernel thread argument */
};

/* ============================================
 * Trap Frame (pt_regs)
 *
 * All registers saved when entering kernel.
 * Used for system calls and exceptions.
 * ============================================ */
struct trapframe {
    /* General purpose registers */
    unsigned long ra;       /* x1 */
    unsigned long sp;       /* x2 */
    unsigned long gp;       /* x3 */
    unsigned long tp;       /* x4 */
    unsigned long t0;       /* x5 */
    unsigned long t1;       /* x6 */
    unsigned long t2;       /* x7 */
    unsigned long s0;       /* x8 / fp */
    unsigned long s1;       /* x9 */
    unsigned long a0;       /* x10 - arg0/return value */
    unsigned long a1;       /* x11 - arg1/return value */
    unsigned long a2;       /* x12 - arg2 */
    unsigned long a3;       /* x13 - arg3 */
    unsigned long a4;       /* x14 - arg4 */
    unsigned long a5;       /* x15 - arg5 */
    unsigned long a6;       /* x16 - arg6 */
    unsigned long a7;       /* x17 - syscall number */
    unsigned long s2;       /* x18 */
    unsigned long s3;       /* x19 */
    unsigned long s4;       /* x20 */
    unsigned long s5;       /* x21 */
    unsigned long s6;       /* x22 */
    unsigned long s7;       /* x23 */
    unsigned long s8;       /* x24 */
    unsigned long s9;       /* x25 */
    unsigned long s10;      /* x26 */
    unsigned long s11;      /* x27 */
    unsigned long t3;       /* x28 */
    unsigned long t4;       /* x29 */
    unsigned long t5;       /* x30 */
    unsigned long t6;       /* x31 */
    /* Control/Status */
    unsigned long sepc;     /* Exception PC */
    unsigned long sstatus;  /* Status register */
    unsigned long scause;   /* Exception cause */
    unsigned long stval;    /* Exception value */
};

#define PT_REGS_SIZE    (36 * 8)

/* Aliases for compatibility */
#define pt_regs trapframe

/* ============================================
 * File Descriptor
 * ============================================ */
typedef struct file_desc {
    int type;               /* File type */
    int ref;                /* Reference count */
    char readable;          /* Readable flag */
    char writable;          /* Writable flag */
    void *data;             /* File-specific data */
    unsigned long off;      /* File offset */
} file_desc_t;

/* ============================================
 * Files Structure
 * ============================================ */
struct files_struct {
    atomic_t count;                     /* Reference count */
    spinlock_t lock;                    /* Lock */
    int max_fds;                        /* Max fd count */
    file_desc_t *fd_array[MAX_OPEN_FILES]; /* File descriptors */
    unsigned long open_fds;             /* Bitmap of open fds */
    unsigned long close_on_exec;        /* Close-on-exec bitmap */
};

/* ============================================
 * Filesystem Info
 * ============================================ */
struct fs_struct {
    atomic_t count;         /* Reference count */
    spinlock_t lock;        /* Lock */
    void *root;             /* Root dentry */
    void *pwd;              /* Current directory */
    unsigned long umask;    /* File creation mask */
};

/* ============================================
 * Signal Types (simplified)
 * ============================================ */
typedef unsigned long sigset_t;

struct sigpending {
    struct list_head list;  /* Pending signal list */
    sigset_t signal;        /* Pending signal bitmap */
};

/* ============================================
 * Wait Queue
 * ============================================ */
typedef struct {
    spinlock_t lock;
    struct list_head head;
} wait_queue_head_t;

#define DECLARE_WAIT_QUEUE_HEAD(name) \
    wait_queue_head_t name = { .lock = SPIN_LOCK_INIT, .head = LIST_HEAD_INIT(name.head) }

static inline void init_waitqueue_head(wait_queue_head_t *q)
{
    spin_lock_init(&q->lock);
    INIT_LIST_HEAD(&q->head);
}

/* ============================================
 * Task Structure (PCB)
 *
 * This is the main process control block.
 * ============================================ */
struct task_struct {
    /* ========== Scheduling ========== */
    volatile long state;            /* Process state (TASK_*) */
    unsigned int flags;             /* Process flags (PF_*) */
    int on_rq;                      /* Is on run queue? */

    int prio;                       /* Dynamic priority */
    int static_prio;                /* Static priority */
    int normal_prio;                /* Normal priority */
    unsigned int rt_priority;       /* Real-time priority */
    unsigned int policy;            /* Scheduling policy */

    struct sched_entity se;         /* Scheduling entity */
    unsigned long time_slice;       /* Remaining time slice */
    unsigned long utime;            /* User CPU time */
    unsigned long stime;            /* System CPU time */
    unsigned long nvcsw;            /* Voluntary context switches */
    unsigned long nivcsw;           /* Involuntary context switches */

    /* ========== Process Identity ========== */
    pid_t pid;                      /* Process ID */
    pid_t tgid;                     /* Thread group ID */
    pid_t ppid;                     /* Parent process ID */

    /* ========== Credentials ========== */
    uid_t uid, euid, suid;          /* User IDs */
    gid_t gid, egid, sgid;          /* Group IDs */

    /* ========== Memory Management ========== */
    struct mm_struct *mm;           /* User address space */
    struct mm_struct *active_mm;    /* Active address space */

    /* ========== Kernel Stack ========== */
    void *stack;                    /* Kernel stack (thread_info) */
    unsigned long kernel_sp;        /* Kernel stack pointer */
    unsigned long user_sp;          /* User stack pointer */

    /* ========== CPU Context ========== */
    struct trapframe *trapframe;    /* Trap frame pointer */
    struct context context;         /* Kernel context */
    struct thread_struct thread;    /* Thread state */

    /* ========== File System ========== */
    struct fs_struct *fs;           /* Filesystem info */
    struct files_struct *files;     /* Open files */
    void *cwd;                      /* Current directory (legacy) */
    file_desc_t *ofile[MAX_OPEN_FILES]; /* Open files (legacy) */

    /* ========== Signals ========== */
    sigset_t blocked;               /* Blocked signals */
    struct sigpending pending;      /* Pending signals */

    /* ========== Process Relations ========== */
    struct task_struct *parent;     /* Parent process */
    struct task_struct *real_parent;/* Real parent */
    struct list_head children;      /* Child list */
    struct list_head sibling;       /* Sibling list link */

    /* ========== Process Group/Session ========== */
    pid_t pgrp;                     /* Process group ID */
    pid_t session;                  /* Session ID */
    struct task_struct *group_leader; /* Thread group leader */

    /* ========== Exit/Wait ========== */
    void *chan;                     /* Sleep channel */
    int killed;                     /* Kill flag */
    int exit_code;                  /* Exit code */
    int exit_signal;                /* Signal on exit */
    wait_queue_head_t wait_chldexit;/* Wait for child exit */

    /* ========== Timing ========== */
    u64 start_time;                 /* Start time (ns) */
    u64 real_start_time;            /* Monotonic start time */

    /* ========== Identification ========== */
    char comm[TASK_COMM_LEN];       /* Executable name */

    /* ========== Lists ========== */
    struct list_head tasks;         /* All processes list */
    struct list_head run_list;      /* Run queue list */
};

/* ============================================
 * Global Variables
 * ============================================ */

/* Process table */
extern struct task_struct *task_table[MAX_PROCS];

/* Init task (PID 0 - idle/swapper) */
extern struct task_struct init_task;

/* All tasks list head */
extern struct list_head all_tasks;

/* ============================================
 * Process Management Functions
 * ============================================ */

/* Initialization */
void proc_init(void);
void fork_init(void);

/* Process allocation */
struct task_struct *alloc_task_struct(void);
void free_task_struct(struct task_struct *tsk);

/* PID management */
pid_t alloc_pid(void);
void free_pid(pid_t pid);
struct task_struct *find_task_by_pid(pid_t pid);

/* Process operations */
pid_t do_fork(unsigned long clone_flags, unsigned long stack_start,
              struct trapframe *regs);
struct task_struct *copy_process(unsigned long clone_flags,
                                  unsigned long stack_start,
                                  struct trapframe *regs);

/* Fork helpers */
int copy_mm(unsigned long clone_flags, struct task_struct *p);
int copy_files(unsigned long clone_flags, struct task_struct *p);
int copy_fs(unsigned long clone_flags, struct task_struct *p);
int copy_sighand(unsigned long clone_flags, struct task_struct *p);
int copy_thread(unsigned long clone_flags, unsigned long usp,
                struct trapframe *regs, struct task_struct *p);

/* Exec */
int do_execve(const char *filename, char **argv, char **envp);
int kernel_execve(const char *filename, char **argv, char **envp);

/* Exit */
void do_exit(long code);
void release_task(struct task_struct *p);

/* Wait */
long do_wait(pid_t pid, int *stat_addr, int options);

/* Process info */
struct task_struct *get_current(void);
void set_current(struct task_struct *p);

/* Legacy compatibility */
#define get_current_proc get_current
#define set_current_proc set_current
#define proc proc_legacy
struct proc_legacy {
    int state;
    pid_t pid;
    pid_t parent;
    unsigned long kstack;
    unsigned long sz;
    unsigned long pagetable;
    struct trapframe *trapframe;
    struct context context;
    void *chan;
    int killed;
    int exit_status;
    char name[16];
    file_desc_t *ofile[MAX_OPEN_FILES];
    void *cwd;
};

/* ============================================
 * Context Switch
 * ============================================ */

/* Low-level context switch (assembly) */
void swtch(struct context *old, struct context *new);

/* Switch to next task */
void switch_to(struct task_struct *prev, struct task_struct *next);

/* Switch address space */
void switch_mm(struct mm_struct *prev_mm, struct mm_struct *next_mm,
               struct task_struct *next);

/* Return from fork (assembly entry point) */
void ret_from_fork(void);

/* Return to user mode */
void ret_to_user(void);

/* ============================================
 * Kernel Threads
 * ============================================ */

/* Create kernel thread */
pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags);

/* CPU idle loop */
void cpu_idle(void);

/* ============================================
 * Helper Macros
 * ============================================ */

/* Error pointer encoding */
#define MAX_ERRNO       4095
#define IS_ERR_VALUE(x) ((unsigned long)(x) >= (unsigned long)(-MAX_ERRNO))
#define IS_ERR(ptr)     IS_ERR_VALUE((unsigned long)(ptr))
#define PTR_ERR(ptr)    ((long)(ptr))
#define ERR_PTR(err)    ((void *)(long)(err))

/* Get pt_regs at top of kernel stack */
static inline struct trapframe *task_pt_regs(struct task_struct *task)
{
    unsigned long stack_top = (unsigned long)task->stack + THREAD_SIZE;
    return (struct trapframe *)(stack_top - PT_REGS_SIZE);
}

/* Get current pt_regs */
#define current_pt_regs()   task_pt_regs(get_current())

#endif /* _MINIX_TASK_H */
