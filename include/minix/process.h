/* Process management */

#ifndef _MINIX_PROCESS_H
#define _MINIX_PROCESS_H

#include <types.h>

/* Process states */
#define PROC_UNUSED     0
#define PROC_EMBRYO     1
#define PROC_RUNNABLE   2
#define PROC_RUNNING    3
#define PROC_SLEEPING   4
#define PROC_ZOMBIE     5

/* Maximum number of processes */
#define MAX_PROCS       64

/* Maximum number of open files per process */
#define MAX_OPEN_FILES  16

/* Process ID type */
typedef int pid_t;

/* Context structure - saved registers */
struct context {
    unsigned long ra;   /* Return address */
    unsigned long sp;   /* Stack pointer */
    unsigned long s0;   /* Saved registers */
    unsigned long s1;
    unsigned long s2;
    unsigned long s3;
    unsigned long s4;
    unsigned long s5;
    unsigned long s6;
    unsigned long s7;
    unsigned long s8;
    unsigned long s9;
    unsigned long s10;
    unsigned long s11;
};

/* Trap frame - saved when entering kernel */
struct trapframe {
    unsigned long ra;
    unsigned long sp;
    unsigned long gp;
    unsigned long tp;
    unsigned long t0;
    unsigned long t1;
    unsigned long t2;
    unsigned long s0;
    unsigned long s1;
    unsigned long a0;
    unsigned long a1;
    unsigned long a2;
    unsigned long a3;
    unsigned long a4;
    unsigned long a5;
    unsigned long a6;
    unsigned long a7;
    unsigned long s2;
    unsigned long s3;
    unsigned long s4;
    unsigned long s5;
    unsigned long s6;
    unsigned long s7;
    unsigned long s8;
    unsigned long s9;
    unsigned long s10;
    unsigned long s11;
    unsigned long t3;
    unsigned long t4;
    unsigned long t5;
    unsigned long t6;
    unsigned long epc;      /* Exception PC */
    unsigned long cause;    /* Exception cause */
    unsigned long tval;     /* Exception value */
    unsigned long status;   /* Status register */
};

/* File descriptor */
typedef struct file_desc {
    int type;           /* File type */
    int ref;            /* Reference count */
    char readable;
    char writable;
    void *data;         /* File-specific data */
    unsigned long off;  /* File offset */
} file_desc_t;

/* Process control block */
struct proc {
    int state;                          /* Process state */
    pid_t pid;                          /* Process ID */
    pid_t parent;                       /* Parent process ID */

    unsigned long kstack;               /* Kernel stack */
    unsigned long sz;                   /* Process size */
    unsigned long pagetable;            /* Page table */

    struct trapframe *trapframe;        /* Trap frame */
    struct context context;             /* Saved context */

    void *chan;                         /* Sleep channel */
    int killed;                         /* Killed flag */
    int exit_status;                    /* Exit status */

    char name[16];                      /* Process name */

    file_desc_t *ofile[MAX_OPEN_FILES]; /* Open files */
    void *cwd;                          /* Current directory */
};

/* Process table */
extern struct proc proc_table[MAX_PROCS];

/* Process management functions */
void proc_init(void);
struct proc *alloc_proc(void);
void free_proc(struct proc *p);
struct proc *get_current_proc(void);
pid_t fork(void);
int exec(const char *path, char **argv);
void exit(int status);
int wait(int *status);
int kill(pid_t pid);
void yield(void);
void sleep(void *chan);
void wakeup(void *chan);

/* Scheduler */
void scheduler(void);
void sched(void);

/* Context switch */
void swtch(struct context *old, struct context *new);

#endif /* _MINIX_PROCESS_H */
