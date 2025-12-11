/* MinixRV64 Donz Build - Process Exit and Wait
 *
 * Process termination and parent notification
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

/* Error codes */
#define ECHILD      10      /* No child processes */
#define EINTR       4       /* Interrupted */

/* Wait options */
#define WNOHANG     1       /* Don't block */
#define WUNTRACED   2       /* Report stopped children */

/* Wait status macros */
#define WEXITSTATUS(s)  (((s) >> 8) & 0xff)
#define WTERMSIG(s)     ((s) & 0x7f)
#define WIFEXITED(s)    (WTERMSIG(s) == 0)
#define WIFSIGNALED(s)  (WTERMSIG(s) != 0 && WTERMSIG(s) != 0x7f)
#define WIFSTOPPED(s)   (WTERMSIG(s) == 0x7f)

/* External functions */
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);
extern void kfree(void *ptr);

/* External variables */
extern struct task_struct *task_table[];
extern struct task_struct init_task;

/* ============================================
 * Exit Memory Management
 * ============================================ */

/* Release process memory space */
void exit_mm(struct task_struct *tsk)
{
    struct mm_struct *mm = tsk->mm;

    if (!mm) return;

    tsk->mm = NULL;

    /* Decrease reference count */
    if (atomic_dec_and_test(&mm->mm_users)) {
        /* Last user - free mm */
        /* TODO: exit_mmap(mm) - free all VMAs */
        /* TODO: pgd_free(mm->pgd) - free page table */
        mm_free(mm);
    }
}

/* ============================================
 * Exit File Descriptors
 * ============================================ */

void exit_files(struct task_struct *tsk)
{
    int i;

    /* Close all open files */
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (tsk->ofile[i]) {
            /* TODO: Properly close and free file descriptor */
            tsk->ofile[i] = NULL;
        }
    }

    /* Release files_struct if allocated */
    if (tsk->files) {
        if (atomic_dec_and_test(&tsk->files->count)) {
            /* TODO: Free files_struct */
        }
        tsk->files = NULL;
    }
}

/* ============================================
 * Exit Filesystem Info
 * ============================================ */

void exit_fs(struct task_struct *tsk)
{
    if (tsk->fs) {
        if (atomic_dec_and_test(&tsk->fs->count)) {
            /* TODO: Free fs_struct */
        }
        tsk->fs = NULL;
    }
    tsk->cwd = NULL;
}

/* ============================================
 * Exit Signal Handling
 * ============================================ */

void exit_sighand(struct task_struct *tsk)
{
    /* Clear pending signals */
    tsk->blocked = 0;
    tsk->pending.signal = 0;
    INIT_LIST_HEAD(&tsk->pending.list);
}

/* ============================================
 * Reparent Children to Init
 * ============================================ */

static void exit_notify(struct task_struct *tsk)
{
    struct task_struct *child, *tmp;

    /* Move all children to init */
    list_for_each_entry_safe(child, tmp, &tsk->children, sibling) {
        child->parent = &init_task;
        child->real_parent = &init_task;

        /* Move to init's children list */
        list_del(&child->sibling);
        list_add_tail(&child->sibling, &init_task.children);

        /* If child is zombie, wake up init */
        if (child->state == TASK_ZOMBIE) {
            /* TODO: Wake up init process */
        }
    }
}

/* ============================================
 * Main Exit Function
 * ============================================ */

void do_exit(long code)
{
    struct task_struct *tsk = get_current();

    if (!tsk) {
        early_puts("[EXIT] No current process\n");
        while (1) {
            asm volatile ("wfi");
        }
    }

    early_puts("[EXIT] Process ");
    early_puthex(tsk->pid);
    early_puts(" exiting with code ");
    early_puthex(code);
    early_puts("\n");

    /* Mark as exiting */
    tsk->flags |= PF_EXITING;
    tsk->exit_code = code;

    /* 1. Release memory */
    exit_mm(tsk);

    /* 2. Close files */
    exit_files(tsk);

    /* 3. Release filesystem info */
    exit_fs(tsk);

    /* 4. Release signal handling */
    exit_sighand(tsk);

    /* 5. Reparent children to init */
    exit_notify(tsk);

    /* 6. Become zombie */
    tsk->state = TASK_ZOMBIE;

    /* 7. Notify parent */
    if (tsk->parent && tsk->parent != tsk) {
        /* Wake up parent if waiting */
        wakeup(&tsk->parent->wait_chldexit);
    }

    /* 8. Give up CPU - never returns */
    schedule();

    /* Should never reach here */
    early_puts("[EXIT] Zombie still running!\n");
    while (1) {
        asm volatile ("wfi");
    }
}

/* ============================================
 * Release Zombie Process
 * ============================================ */

void release_task(struct task_struct *p)
{
    int i;

    if (!p) return;

    /* Remove from parent's children list */
    list_del(&p->sibling);

    /* Remove from all tasks list */
    list_del(&p->tasks);

    /* Remove from process table */
    for (i = 0; i < MAX_PROCS; i++) {
        if (task_table[i] == p) {
            task_table[i] = NULL;
            break;
        }
    }

    /* Free PID */
    free_pid(p->pid);

    /* Free kernel stack */
    if (p->stack) {
        free_thread_info((struct thread_info *)p->stack);
    }

    /* Free task_struct */
    free_task_struct(p);
}

/* ============================================
 * Check for Zombie Children
 * ============================================ */

#if 0  /* Currently unused, but may be needed later */
static int has_zombie_child(struct task_struct *parent)
{
    struct task_struct *child;

    list_for_each_entry(child, &parent->children, sibling) {
        if (child->state == TASK_ZOMBIE) {
            return 1;
        }
    }
    return 0;
}
#endif

/* ============================================
 * Wait for Child Process
 * ============================================ */

long do_wait(pid_t pid, int *stat_addr, int options)
{
    struct task_struct *curr = get_current();
    struct task_struct *child;
    int retval;
    pid_t child_pid;

    if (!curr) return -ECHILD;

repeat:
    retval = -ECHILD;

    /* Search for matching child */
    list_for_each_entry(child, &curr->children, sibling) {
        /* Match PID criteria */
        if (pid > 0 && child->pid != pid)
            continue;
        if (pid == 0 && child->pgrp != curr->pgrp)
            continue;
        if (pid < -1 && child->pgrp != -pid)
            continue;

        /* Found a child */
        if (child->state == TASK_ZOMBIE) {
            /* Child has exited */
            child_pid = child->pid;

            /* Return exit status */
            if (stat_addr) {
                *stat_addr = child->exit_code;
            }

            /* Release the zombie */
            release_task(child);

            return child_pid;
        }

        /* Have children but none are zombies yet */
        retval = 0;
    }

    /* No children at all */
    if (retval == -ECHILD) {
        return -ECHILD;
    }

    /* WNOHANG: Don't block */
    if (options & WNOHANG) {
        return 0;
    }

    /* Sleep waiting for child to exit */
    curr->chan = &curr->wait_chldexit;
    curr->state = TASK_INTERRUPTIBLE;
    schedule();

    /* Check if we were interrupted */
    if (curr->killed) {
        return -EINTR;
    }

    goto repeat;
}

/* ============================================
 * System Call Wrappers
 * ============================================ */

/* exit() system call */
long sys_exit(int error_code)
{
    do_exit((error_code & 0xff) << 8);
    return 0;  /* Never returns */
}

/* exit_group() system call - terminate thread group */
long sys_exit_group(int error_code)
{
    /* For now, same as exit */
    do_exit((error_code & 0xff) << 8);
    return 0;
}

/* wait4() system call */
long sys_wait4(pid_t pid, int *stat_addr, int options, void *rusage)
{
    (void)rusage;  /* TODO: Implement rusage */
    return do_wait(pid, stat_addr, options);
}

/* waitpid() system call */
long sys_waitpid(pid_t pid, int *stat_addr, int options)
{
    return do_wait(pid, stat_addr, options);
}

/* wait() system call */
long sys_wait(int *stat_addr)
{
    return do_wait(-1, stat_addr, 0);
}

/* ============================================
 * Kill Process
 * ============================================ */

int kill_process(pid_t pid)
{
    struct task_struct *p;
    int i;

    for (i = 0; i < MAX_PROCS; i++) {
        p = task_table[i];
        if (p && p->pid == pid) {
            p->killed = 1;

            /* Wake up if sleeping */
            if (p->state == TASK_INTERRUPTIBLE ||
                p->state == TASK_UNINTERRUPTIBLE) {
                wake_up_process(p);
            }
            return 0;
        }
    }

    return -1;  /* Process not found */
}

/* sys_kill() - send signal (simplified) */
long sys_kill(pid_t pid, int sig)
{
    (void)sig;  /* For now, any signal kills */

    if (pid > 0) {
        return kill_process(pid);
    } else if (pid == 0) {
        /* Kill process group */
        struct task_struct *curr = get_current();
        if (curr) {
            /* TODO: Kill all in process group */
        }
        return 0;
    } else if (pid == -1) {
        /* Kill all (except init) */
        /* TODO: Implement */
        return 0;
    } else {
        /* Kill specific process group */
        /* TODO: Implement */
        return 0;
    }
}
