/* Process management implementation */

#include <minix/config.h>
#include <minix/process.h>
#include <types.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Process table */
struct proc proc_table[MAX_PROCS];

/* Current process (per-CPU, simplified for single CPU) */
static struct proc *current_proc = NULL;

/* Next PID to allocate */
static pid_t next_pid = 1;

/* External functions */
extern void early_puts(const char *s);

/* Initialize process subsystem */
void proc_init(void)
{
    int i;

    /* Initialize all processes as unused */
    for (i = 0; i < MAX_PROCS; i++) {
        proc_table[i].state = PROC_UNUSED;
        proc_table[i].pid = 0;
    }

    /* Process management initialized silently */
}

/* Allocate a new process */
struct proc *alloc_proc(void)
{
    int i;
    struct proc *p;

    /* Find an unused process slot */
    for (i = 0; i < MAX_PROCS; i++) {
        p = &proc_table[i];
        if (p->state == PROC_UNUSED) {
            /* Initialize process */
            p->state = PROC_EMBRYO;
            p->pid = next_pid++;
            p->parent = 0;
            p->kstack = 0;
            p->sz = 0;
            p->pagetable = 0;
            p->trapframe = NULL;
            p->chan = NULL;
            p->killed = 0;
            p->exit_status = 0;
            p->cwd = NULL;

            /* Initialize open files */
            for (int j = 0; j < MAX_OPEN_FILES; j++) {
                p->ofile[j] = NULL;
            }

            /* Clear name */
            for (int j = 0; j < 16; j++) {
                p->name[j] = 0;
            }

            return p;
        }
    }

    return NULL;  /* No free process */
}

/* Free a process */
void free_proc(struct proc *p)
{
    if (p == NULL) return;

    /* TODO: Free kernel stack, page table, etc. */

    p->state = PROC_UNUSED;
    p->pid = 0;
}

/* Get current process */
struct proc *get_current_proc(void)
{
    return current_proc;
}

/* Set current process */
void set_current_proc(struct proc *p)
{
    current_proc = p;
}

/* Copy memory from parent to child (simplified) */
static int copy_memory(struct proc *parent, struct proc *child)
{
    /* For now, just copy the size - actual page table copying would go here */
    child->sz = parent->sz;
    /* TODO: Copy page tables and physical memory */
    return 0;
}

/* Copy file descriptors from parent to child */
static void copy_files(struct proc *parent, struct proc *child)
{
    int i;
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (parent->ofile[i] != NULL) {
            child->ofile[i] = parent->ofile[i];
            /* TODO: Increment reference count */
        }
    }
}

/* Fork - create a new process */
pid_t fork(void)
{
    struct proc *parent = get_current_proc();
    struct proc *child;

    if (parent == NULL) {
        return -1;
    }

    /* Allocate new process */
    child = alloc_proc();
    if (child == NULL) {
        return -1;  /* No free process slots */
    }

    /* Set parent */
    child->parent = parent->pid;

    /* Copy memory */
    if (copy_memory(parent, child) != 0) {
        free_proc(child);
        return -1;
    }

    /* Copy file descriptors */
    copy_files(parent, child);

    /* Copy current directory */
    child->cwd = parent->cwd;

    /* Copy process name */
    for (int i = 0; i < 16; i++) {
        child->name[i] = parent->name[i];
    }

    /* Allocate trapframe and copy from parent */
    /* TODO: Allocate actual trapframe memory */
    child->trapframe = parent->trapframe;

    /* Set return value for child to 0 */
    if (child->trapframe != NULL) {
        child->trapframe->a0 = 0;
    }

    /* Make child runnable */
    child->state = PROC_RUNNABLE;

    /* Return child PID to parent */
    return child->pid;
}

/* Exec - replace process image */
int exec(const char *path, char **argv)
{
    struct proc *p = get_current_proc();

    if (p == NULL || path == NULL) {
        return -1;
    }

    /* TODO: Load executable file from path */
    /* TODO: Parse ELF header and load segments */
    /* TODO: Setup stack with argc/argv */
    /* TODO: Setup trapframe for new program entry point */

    /* For now, just validate the path and arguments exist */
    (void)argv;  /* Suppress warning until implemented */

    /* Update process name from path */
    const char *name = path;
    const char *p_char = path;

    /* Find last '/' to get program name */
    while (*p_char != '\0') {
        if (*p_char == '/') {
            name = p_char + 1;
        }
        p_char++;
    }

    /* Copy program name */
    int i;
    for (i = 0; i < 15 && name[i] != '\0'; i++) {
        p->name[i] = name[i];
    }
    p->name[i] = '\0';

    /* TODO: Actually load and execute the program */
    early_puts("exec: loading ");
    early_puts(path);
    early_puts(" (not fully implemented)\n");

    return -1;  /* Return -1 until fully implemented */
}

/* Exit - terminate process */
void exit(int status)
{
    struct proc *p = get_current_proc();

    if (p == NULL) {
        early_puts("exit: no current process\n");
        return;
    }

    p->exit_status = status;
    p->state = PROC_ZOMBIE;

    /* TODO: Wake up parent */
    /* TODO: Reparent children */

    /* Give up CPU */
    yield();

    /* Should not reach here */
    early_puts("exit: zombie process still running!\n");
    while (1) {
        asm volatile ("wfi");
    }
}

/* Wait - wait for child process */
int wait(int *status)
{
    struct proc *p = get_current_proc();
    struct proc *child;
    int i, have_children;
    pid_t child_pid;

    if (p == NULL) {
        return -1;
    }

    while (1) {
        have_children = 0;

        /* Look for zombie children */
        for (i = 0; i < MAX_PROCS; i++) {
            child = &proc_table[i];

            if (child->parent == p->pid) {
                have_children = 1;

                if (child->state == PROC_ZOMBIE) {
                    /* Found a zombie child - reap it */
                    child_pid = child->pid;

                    /* Copy exit status if requested */
                    if (status != NULL) {
                        *status = child->exit_status;
                    }

                    /* Free the child process */
                    free_proc(child);

                    return child_pid;
                }
            }
        }

        /* No children at all */
        if (!have_children) {
            return -1;
        }

        /* Have children but none are zombies - sleep and wait */
        /* TODO: Implement sleep/wakeup properly */
        /* For now, just yield and check again */
        yield();
    }
}

/* Kill - send signal to process */
int kill(pid_t pid)
{
    int i;
    struct proc *p;

    for (i = 0; i < MAX_PROCS; i++) {
        p = &proc_table[i];
        if (p->pid == pid) {
            p->killed = 1;
            if (p->state == PROC_SLEEPING) {
                p->state = PROC_RUNNABLE;
            }
            return 0;
        }
    }

    return -1;  /* Process not found */
}

/* Note: yield() is implemented in sched.c */

/* Sleep - sleep on channel */
void sleep(void *chan)
{
    struct proc *p = get_current_proc();

    if (p == NULL) return;

    p->chan = chan;
    p->state = PROC_SLEEPING;

    /* Give up CPU */
    yield();
}

/* Wakeup - wake up processes sleeping on channel */
void wakeup(void *chan)
{
    int i;
    struct proc *p;

    for (i = 0; i < MAX_PROCS; i++) {
        p = &proc_table[i];
        if (p->state == PROC_SLEEPING && p->chan == chan) {
            p->state = PROC_RUNNABLE;
            p->chan = NULL;
        }
    }
}

/* Simple round-robin scheduler */
void scheduler(void)
{
    static int last_proc = 0;
    int i;
    struct proc *p;

    /* Find next runnable process */
    for (i = 0; i < MAX_PROCS; i++) {
        int idx = (last_proc + i + 1) % MAX_PROCS;
        p = &proc_table[idx];

        if (p->state == PROC_RUNNABLE) {
            /* Switch to this process */
            last_proc = idx;
            p->state = PROC_RUNNING;
            set_current_proc(p);

            /* TODO: Context switch */
            /* For now, just return */
            return;
        }
    }

    /* No runnable process, idle */
    set_current_proc(NULL);
}

/* Schedule - called from trap or yield */
void sched(void)
{
    struct proc *p = get_current_proc();

    if (p != NULL && p->state == PROC_RUNNING) {
        p->state = PROC_RUNNABLE;
    }

    /* Run scheduler */
    scheduler();
}
