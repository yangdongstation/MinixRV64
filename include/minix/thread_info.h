/* MinixRV64 Donz Build - Thread Info and Kernel Stack
 *
 * Per-thread information structure at bottom of kernel stack
 * Following HowToFitPosix.md Stage 2 design
 */

#ifndef _MINIX_THREAD_INFO_H
#define _MINIX_THREAD_INFO_H

#include <types.h>

/* Forward declaration */
struct task_struct;

/* ============================================
 * Kernel Stack Size
 * ============================================ */
#define THREAD_SIZE         (2 * 4096)  /* 8KB per process */
#define THREAD_SIZE_ORDER   1           /* 2 pages */
#define THREAD_MASK         (~(THREAD_SIZE - 1))

/* ============================================
 * Thread Info Flags
 * ============================================ */
#define TIF_SIGPENDING      0   /* Signal pending */
#define TIF_NEED_RESCHED    1   /* Rescheduling needed */
#define TIF_SYSCALL_TRACE   2   /* System call tracing */
#define TIF_NOTIFY_RESUME   3   /* Callback before returning to user */
#define TIF_RESTORE_SIGMASK 4   /* Restore signal mask */

#define _TIF_SIGPENDING     (1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED   (1 << TIF_NEED_RESCHED)
#define _TIF_SYSCALL_TRACE  (1 << TIF_SYSCALL_TRACE)
#define _TIF_NOTIFY_RESUME  (1 << TIF_NOTIFY_RESUME)

#define _TIF_WORK_MASK      (_TIF_SIGPENDING | _TIF_NEED_RESCHED | _TIF_NOTIFY_RESUME)

/* ============================================
 * Address Space Segment
 * ============================================ */
typedef struct {
    unsigned long seg;
} mm_segment_t;

#define KERNEL_DS   ((mm_segment_t) { 0 })
#define USER_DS     ((mm_segment_t) { ~0UL })

/* ============================================
 * Thread Info Structure
 *
 * This structure lives at the bottom of the kernel stack.
 * Layout:
 *
 * High address
 * ┌─────────────────────────────────┐ ← Stack top + THREAD_SIZE
 * │        struct pt_regs           │   (trap frame at top)
 * ├─────────────────────────────────┤
 * │                                 │
 * │      Kernel stack space         │   (grows down)
 * │              ↓                  │
 * │                                 │
 * ├─────────────────────────────────┤
 * │      struct thread_info         │   ← Stack bottom
 * └─────────────────────────────────┘
 * Low address
 *
 * ============================================ */
struct thread_info {
    struct task_struct *task;       /* Main task structure */
    unsigned long flags;            /* Thread flags (TIF_*) */
    int preempt_count;              /* Preemption counter */
    int cpu;                        /* Current CPU */
    mm_segment_t addr_limit;        /* Address space limit */
    unsigned long kernel_sp;        /* Kernel stack pointer */
    unsigned long user_sp;          /* User stack pointer (saved) */
};

/* ============================================
 * Thread Info Access
 * ============================================ */

/* Get thread_info from stack pointer */
static inline struct thread_info *current_thread_info(void)
{
    unsigned long sp;
    asm volatile ("mv %0, sp" : "=r"(sp));
    return (struct thread_info *)(sp & THREAD_MASK);
}

/* Get task from thread_info */
#define current (current_thread_info()->task)

/* Thread flag operations */
static inline void set_ti_thread_flag(struct thread_info *ti, int flag)
{
    ti->flags |= (1UL << flag);
}

static inline void clear_ti_thread_flag(struct thread_info *ti, int flag)
{
    ti->flags &= ~(1UL << flag);
}

static inline int test_ti_thread_flag(struct thread_info *ti, int flag)
{
    return (ti->flags & (1UL << flag)) != 0;
}

/* Convenience macros */
#define set_thread_flag(flag)   set_ti_thread_flag(current_thread_info(), flag)
#define clear_thread_flag(flag) clear_ti_thread_flag(current_thread_info(), flag)
#define test_thread_flag(flag)  test_ti_thread_flag(current_thread_info(), flag)

/* Preemption */
#define preempt_count()         (current_thread_info()->preempt_count)
#define preempt_disable()       do { preempt_count()++; } while (0)
#define preempt_enable()        do { preempt_count()--; } while (0)
#define preemptible()           (preempt_count() == 0)

/* ============================================
 * Kernel Stack Allocation
 * ============================================ */

/* Allocate kernel stack + thread_info */
struct thread_info *alloc_thread_info(void);

/* Free kernel stack + thread_info */
void free_thread_info(struct thread_info *ti);

/* Initialize thread_info for a new task */
void setup_thread_info(struct thread_info *ti, struct task_struct *task);

#endif /* _MINIX_THREAD_INFO_H */
