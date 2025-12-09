/* Minix RV64 scheduler */

#include <minix/config.h>
#include <minix/process.h>
#include <types.h>

/* Define NULL */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* External functions */
extern void early_puts(const char *s);

/* Initialize scheduler */
void sched_init(void)
{
    /* Process table is initialized in proc_init() */
    early_puts("✓ Scheduler\n");
}

/* Yield CPU - give up current timeslice */
void yield(void)
{
    struct proc *p = get_current_proc();

    if (p != NULL && p->state == PROC_RUNNING) {
        p->state = PROC_RUNNABLE;
    }

    /* Call scheduler to pick next process */
    scheduler();
}