/* MinixRV64 Donz Build - Init Process Setup
 *
 * Initialize idle process (PID 0) and init process (PID 1)
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

/* External variables */
extern struct task_struct init_task;
extern struct task_struct *task_table[];

/* External scheduler functions */
extern void sched_init(void);

/* Linker symbols */
extern unsigned long __stack_end;

/* ============================================
 * Idle Process (PID 0)
 *
 * The idle process runs when no other process
 * is ready. It simply waits for interrupts.
 * Note: Not currently used as kernel thread, but
 * provides the idle loop functionality.
 * ============================================ */

#if 0  /* Will be used when we have proper thread creation */
static int idle_thread(void *unused)
{
    (void)unused;

    early_puts("[IDLE] Idle process started\n");

    while (1) {
        /* Check for reschedule */
        if (need_resched()) {
            schedule();
        }
        /* Wait for interrupt */
        asm volatile ("wfi");
    }

    return 0;
}
#endif

/* ============================================
 * Init Process (PID 1)
 *
 * The init process is the ancestor of all user
 * processes. It spawns the shell and reaps
 * orphaned processes.
 * ============================================ */

static int init_thread(void *unused)
{
    (void)unused;

    early_puts("[INIT] Init process started (PID 1)\n");

    /* For now, just run the shell as PID 1's work */
    /* In full implementation, this would:
     * 1. Mount root filesystem
     * 2. Open /dev/console for stdin/stdout/stderr
     * 3. Execute /sbin/init or /etc/rc
     * 4. Wait for children and reap zombies
     */

    /* Call shell_run from the init context */
    extern void shell_run(void);
    shell_run();

    /* If shell exits, just wait */
    while (1) {
        /* Reap any zombie children */
        long pid;
        int status;
        pid = do_wait(-1, &status, 1);  /* WNOHANG */
        if (pid <= 0) {
            /* No zombies, sleep briefly */
            schedule();
        }
    }

    return 0;
}

/* ============================================
 * Setup Idle Process (PID 0)
 *
 * init_task is statically allocated in fork.c.
 * We just need to finish setting it up.
 * ============================================ */

void setup_idle_process(void)
{
    struct thread_info *ti;

    early_puts("[PROC] Setting up idle process (PID 0)\n");

    /* Allocate kernel stack for idle if not already done */
    if (!init_task.stack) {
        ti = alloc_thread_info();
        if (ti) {
            ti->task = &init_task;
            init_task.stack = ti;
            ti->kernel_sp = (unsigned long)ti + THREAD_SIZE;
        }
    }

    /* Set as current task */
    set_current(&init_task);

    early_puts("[PROC] Idle process ready\n");
}

/* ============================================
 * Create Init Process (PID 1)
 * ============================================ */

void create_init_process(void)
{
    pid_t pid;

    early_puts("[PROC] Creating init process (PID 1)\n");

    /* Create init as a kernel thread */
    pid = kernel_thread(init_thread, NULL, 0);

    if (pid < 0) {
        early_puts("[PROC] ERROR: Failed to create init process!\n");
        return;
    }

    /* Find init in task table and set special properties */
    struct task_struct *init = find_task_by_pid(pid);
    if (init) {
        /* Set name */
        init->comm[0] = 'i';
        init->comm[1] = 'n';
        init->comm[2] = 'i';
        init->comm[3] = 't';
        init->comm[4] = '\0';

        /* Init is its own session leader */
        init->session = pid;
        init->pgrp = pid;

        early_puts("[PROC] Init process created with PID ");
        early_puthex(pid);
        early_puts("\n");
    }
}

/* ============================================
 * Start Process Subsystem
 *
 * Called from main.c to initialize Stage 2
 * process management.
 * ============================================ */

void proc_subsystem_init(void)
{
    early_puts("\n=== Stage 2: Process Management ===\n");

    /* 1. Initialize fork subsystem (PID bitmap, task cache) */
    fork_init();

    /* 2. Initialize new O(1) scheduler */
    sched_init();

    /* 3. Setup idle process (PID 0) */
    setup_idle_process();

    early_puts("=== Process subsystem ready ===\n\n");
}

/* ============================================
 * Start Init Process
 *
 * Called after all drivers are initialized.
 * Creates PID 1 and starts the system.
 * ============================================ */

void start_init(void)
{
    /* Create init process (PID 1) */
    create_init_process();

    /* Enable preemption and let scheduler take over */
    /* For now, we just call schedule to let init run */
    early_puts("[PROC] Starting init process...\n");

    /* Schedule - this will switch to init */
    schedule();

    /* If we return here, fall into idle loop */
    early_puts("[PROC] Entering idle loop\n");
    cpu_idle();
}
