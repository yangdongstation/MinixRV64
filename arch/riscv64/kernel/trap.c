/* RISC-V trap and exception handling */

#include <minix/config.h>
#include <asm/csr.h>
#include <types.h>

/* Trap frame structure */
struct trap_frame {
    unsigned long ra;     /* x1  */
    unsigned long sp;     /* x2  */
    unsigned long gp;     /* x3  */
    unsigned long tp;     /* x4  */
    unsigned long t0;     /* x5  */
    unsigned long t1;     /* x6  */
    unsigned long t2;     /* x7  */
    unsigned long s0;     /* x8  */
    unsigned long s1;     /* x9  */
    unsigned long a0;     /* x10 */
    unsigned long a1;     /* x11 */
    unsigned long a2;     /* x12 */
    unsigned long a3;     /* x13 */
    unsigned long a4;     /* x14 */
    unsigned long a5;     /* x15 */
    unsigned long a6;     /* x16 */
    unsigned long a7;     /* x17 */
    unsigned long s2;     /* x18 */
    unsigned long s3;     /* x19 */
    unsigned long s4;     /* x20 */
    unsigned long s5;     /* x21 */
    unsigned long s6;     /* x22 */
    unsigned long s7;     /* x23 */
    unsigned long s8;     /* x24 */
    unsigned long s9;     /* x25 */
    unsigned long s10;    /* x26 */
    unsigned long s11;    /* x27 */
    unsigned long t3;     /* x28 */
    unsigned long t4;     /* x29 */
    unsigned long t5;     /* x30 */
    unsigned long t6;     /* x31 */
    unsigned long sepc;
    unsigned long sstatus;
    unsigned long scause;
    unsigned long stval;
};

/* Forward declarations */
void handle_interrupt(struct trap_frame *tf);
void handle_exception(struct trap_frame *tf);
extern void trap_vector(void);

/* Trap entry point (called from assembly) */
void do_trap(struct trap_frame *tf)
{
    unsigned long cause = tf->scause & 0xFFFFFFFF;

    if (cause & 0x80000000) {
        /* It's an interrupt */
        handle_interrupt(tf);
    } else {
        /* It's an exception */
        handle_exception(tf);
    }
}

/* Handle interrupts */
void handle_interrupt(struct trap_frame *tf)
{
    unsigned long cause = tf->scause & 0x7FFFFFFF;

    switch (cause) {
    case IRQ_S_SOFT:
        /* Software interrupt */
        break;

    case IRQ_S_TIMER:
        /* Timer interrupt */
        /* TODO: Handle timer tick */
        break;

    case IRQ_S_EXT:
        /* External interrupt */
        /* TODO: Handle external devices */
        break;

    default:
        /* Unknown interrupt */
        break;
    }
}

/* Handle exceptions */
void handle_exception(struct trap_frame *tf)
{
    unsigned long cause = tf->scause;

    switch (cause) {
    case EXC_INST_MISALIGNED:
        /* Instruction address misaligned */
        break;

    case EXC_INST_ACCESS:
        /* Instruction access fault */
        break;

    case EXC_INST_ILLEGAL:
        /* Illegal instruction */
        break;

    case EXC_BREAKPOINT:
        /* Breakpoint */
        break;

    case EXC_LOAD_MISALIGNED:
        /* Load address misaligned */
        break;

    case EXC_LOAD_ACCESS:
        /* Load access fault */
        break;

    case EXC_STORE_MISALIGNED:
        /* Store address misaligned */
        break;

    case EXC_STORE_ACCESS:
        /* Store access fault */
        break;

    case EXC_ECALL_U:
        /* Environment call from U-mode */
        /* TODO: Handle system call */
        break;

    case EXC_ECALL_S:
        /* Environment call from S-mode */
        break;

    case EXC_INST_PAGE_FAULT:
        /* Instruction page fault */
        break;

    case EXC_LOAD_PAGE_FAULT:
        /* Load page fault */
        break;

    case EXC_STORE_PAGE_FAULT:
        /* Store page fault */
        break;

    default:
        /* Unknown exception */
        break;
    }
}

/* Initialize trap handling */
void trap_init(void)
{
    /* Set trap vector */
    asm volatile ("csrw stvec, %0" :: "r"(&trap_vector));

    /* Enable supervisor external and timer interrupts */
    set_csr(CSR_SIE, (1 << IRQ_S_EXT) | (1 << IRQ_S_TIMER));
}