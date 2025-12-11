/* RISC-V trap and exception handling */

#include <minix/config.h>
#include <asm/csr.h>
#include <types.h>

/* Trap frame structure - saved registers on exception/interrupt */
struct trap_frame {
    unsigned long ra;     /* x1  - Return address */
    unsigned long sp;     /* x2  - Stack pointer */
    unsigned long gp;     /* x3  - Global pointer */
    unsigned long tp;     /* x4  - Thread pointer */
    unsigned long t0;     /* x5  - Temporary */
    unsigned long t1;     /* x6  - Temporary */
    unsigned long t2;     /* x7  - Temporary */
    unsigned long s0;     /* x8  - Saved register / Frame pointer */
    unsigned long s1;     /* x9  - Saved register */
    unsigned long a0;     /* x10 - Argument / Return value */
    unsigned long a1;     /* x11 - Argument / Return value */
    unsigned long a2;     /* x12 - Argument */
    unsigned long a3;     /* x13 - Argument */
    unsigned long a4;     /* x14 - Argument */
    unsigned long a5;     /* x15 - Argument */
    unsigned long a6;     /* x16 - Argument */
    unsigned long a7;     /* x17 - Argument / Syscall number */
    unsigned long s2;     /* x18 - Saved register */
    unsigned long s3;     /* x19 - Saved register */
    unsigned long s4;     /* x20 - Saved register */
    unsigned long s5;     /* x21 - Saved register */
    unsigned long s6;     /* x22 - Saved register */
    unsigned long s7;     /* x23 - Saved register */
    unsigned long s8;     /* x24 - Saved register */
    unsigned long s9;     /* x25 - Saved register */
    unsigned long s10;    /* x26 - Saved register */
    unsigned long s11;    /* x27 - Saved register */
    unsigned long t3;     /* x28 - Temporary */
    unsigned long t4;     /* x29 - Temporary */
    unsigned long t5;     /* x30 - Temporary */
    unsigned long t6;     /* x31 - Temporary */
    unsigned long sepc;   /* Exception PC */
    unsigned long sstatus;/* Status register */
    unsigned long scause; /* Exception cause */
    unsigned long stval;  /* Trap value (fault address, etc.) */
};

/* Page fault types */
#define FAULT_INST_FETCH    0
#define FAULT_LOAD          1
#define FAULT_STORE         2

/* Forward declarations */
void handle_interrupt(struct trap_frame *tf);
void handle_exception(struct trap_frame *tf);
void do_page_fault(struct trap_frame *tf, int fault_type);
extern void trap_vector(void);
extern void printk(const char *, ...);
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);

/* External memory management functions */
extern unsigned long alloc_page(void);
extern int map_page_4k(void *pgd, unsigned long va, unsigned long pa, unsigned long flags);
extern void *get_kernel_pgd(void);

/* Page table entry flags */
#define PTE_V               (1UL << 0)
#define PTE_R               (1UL << 1)
#define PTE_W               (1UL << 2)
#define PTE_X               (1UL << 3)
#define PTE_U               (1UL << 4)
#define PTE_A               (1UL << 6)
#define PTE_D               (1UL << 7)

/* Page fault counter for debugging */
static unsigned long page_fault_count = 0;

/* Trap entry point (called from assembly) */
void do_trap(struct trap_frame *tf)
{
    unsigned long cause = tf->scause;

    /* High bit set = interrupt, clear = exception */
    if (cause & (1UL << 63)) {
        handle_interrupt(tf);
    } else {
        handle_exception(tf);
    }
}

/* Handle interrupts */
void handle_interrupt(struct trap_frame *tf)
{
    unsigned long cause = tf->scause & 0x7FFFFFFFFFFFFFFFUL;

    switch (cause) {
    case IRQ_S_SOFT:
        /* Software interrupt - used for IPI */
        /* Clear the interrupt */
        /* TODO: Handle software interrupt */
        break;

    case IRQ_S_TIMER:
        /* Timer interrupt - scheduler tick */
        /* TODO: Call scheduler_tick() */
        /* For now, just acknowledge by clearing SIP.STIP via SBI */
        break;

    case IRQ_S_EXT:
        /* External interrupt - PLIC */
        /* TODO: Call PLIC handler to dispatch to device drivers */
        break;

    default:
        early_puts("[IRQ] Unknown interrupt: ");
        early_puthex(cause);
        early_puts("\n");
        break;
    }
}

/* Print detailed trap information */
static void dump_trap_info(struct trap_frame *tf, const char *fault_type)
{
    early_puts("\n========== ");
    early_puts(fault_type);
    early_puts(" ==========\n");

    early_puts("  SEPC (PC):    0x");
    early_puthex(tf->sepc);
    early_puts("\n  STVAL (Addr): 0x");
    early_puthex(tf->stval);
    early_puts("\n  SCAUSE:       0x");
    early_puthex(tf->scause);
    early_puts("\n  SSTATUS:      0x");
    early_puthex(tf->sstatus);
    early_puts("\n");

    /* Decode sstatus */
    early_puts("  Mode: ");
    if (tf->sstatus & SSTATUS_SPP) {
        early_puts("Supervisor");
    } else {
        early_puts("User");
    }
    early_puts("\n");

    /* Print some registers */
    early_puts("  RA:  0x");
    early_puthex(tf->ra);
    early_puts("  SP:  0x");
    early_puthex(tf->sp);
    early_puts("\n  A0:  0x");
    early_puthex(tf->a0);
    early_puts("  A1:  0x");
    early_puthex(tf->a1);
    early_puts("\n");
}

/* Handle page fault */
void do_page_fault(struct trap_frame *tf, int fault_type)
{
    unsigned long fault_addr = tf->stval;
    int is_user = !(tf->sstatus & SSTATUS_SPP);

    /* These variables will be used when we implement demand paging */
    (void)fault_addr;  /* Used in dump_trap_info via tf->stval */

    page_fault_count++;

    /* Determine fault type string */
    const char *type_str;
    switch (fault_type) {
    case FAULT_INST_FETCH:
        type_str = "Instruction Page Fault";
        break;
    case FAULT_LOAD:
        type_str = "Load Page Fault";
        break;
    case FAULT_STORE:
        type_str = "Store Page Fault";
        break;
    default:
        type_str = "Unknown Page Fault";
        break;
    }

    /*
     * For now, we don't have user processes, so all faults in kernel
     * mode are errors. In the future, this would:
     *
     * 1. Check if fault address is in a valid VMA (vm_area_struct)
     * 2. For demand paging: allocate page and map it
     * 3. For COW: copy page and update mapping
     * 4. For stack growth: extend stack VMA
     * 5. Otherwise: send SIGSEGV to user process
     */

    /* For kernel faults, print diagnostic and halt */
    if (!is_user) {
        dump_trap_info(tf, type_str);

        /* Check for common kernel programming errors */
        if (fault_addr == 0) {
            early_puts("\n  >>> NULL pointer dereference! <<<\n");
        } else if (fault_addr < 0x1000) {
            early_puts("\n  >>> Low address access (likely NULL + offset) <<<\n");
        } else if (fault_addr >= 0xFFFFFFFF00000000UL) {
            early_puts("\n  >>> Invalid high address <<<\n");
        }

        /* Halt - kernel bug */
        early_puts("\n  KERNEL PANIC: Unhandled page fault in kernel mode\n");
        early_puts("  System halted.\n");
        while (1) {
            asm volatile("wfi");
        }
    }

    /* User mode fault - for now, just kill the process */
    /* In the future, this would:
     * 1. Look up VMA for fault_addr
     * 2. Check permissions (read/write/exec)
     * 3. Handle demand paging or COW
     * 4. If no valid VMA, send SIGSEGV
     */
    dump_trap_info(tf, type_str);
    early_puts("\n  User process fault - would send SIGSEGV\n");

    /* For now, halt since we don't have process management */
    early_puts("  System halted (no process management yet)\n");
    while (1) {
        asm volatile("wfi");
    }
}

/* Handle exceptions */
void handle_exception(struct trap_frame *tf)
{
    unsigned long cause = tf->scause;

    switch (cause) {
    case EXC_INST_MISALIGNED:
        dump_trap_info(tf, "Instruction Misaligned");
        early_puts("  KERNEL PANIC: Misaligned instruction fetch\n");
        while (1) asm volatile("wfi");
        break;

    case EXC_INST_ACCESS:
        dump_trap_info(tf, "Instruction Access Fault");
        early_puts("  KERNEL PANIC: Cannot fetch instruction from this address\n");
        while (1) asm volatile("wfi");
        break;

    case EXC_INST_ILLEGAL:
        dump_trap_info(tf, "Illegal Instruction");
        early_puts("  KERNEL PANIC: Illegal instruction executed\n");
        while (1) asm volatile("wfi");
        break;

    case EXC_BREAKPOINT:
        /* Breakpoint - used for debugging */
        early_puts("[TRAP] Breakpoint at ");
        early_puthex(tf->sepc);
        early_puts("\n");
        /* Skip the ebreak instruction */
        tf->sepc += 2;  /* ebreak is 2 bytes in compressed ISA, 4 in RV64I */
        break;

    case EXC_LOAD_MISALIGNED:
        dump_trap_info(tf, "Load Misaligned");
        early_puts("  KERNEL PANIC: Misaligned load\n");
        while (1) asm volatile("wfi");
        break;

    case EXC_LOAD_ACCESS:
        dump_trap_info(tf, "Load Access Fault");
        early_puts("  KERNEL PANIC: Cannot load from this address\n");
        while (1) asm volatile("wfi");
        break;

    case EXC_STORE_MISALIGNED:
        dump_trap_info(tf, "Store Misaligned");
        early_puts("  KERNEL PANIC: Misaligned store\n");
        while (1) asm volatile("wfi");
        break;

    case EXC_STORE_ACCESS:
        dump_trap_info(tf, "Store Access Fault");
        early_puts("  KERNEL PANIC: Cannot store to this address\n");
        while (1) asm volatile("wfi");
        break;

    case EXC_ECALL_U:
        /* System call from user mode */
        /* TODO: Implement syscall handler */
        early_puts("[SYSCALL] User syscall ");
        early_puthex(tf->a7);
        early_puts(" not implemented\n");
        tf->a0 = (unsigned long)-38;  /* -ENOSYS */
        tf->sepc += 4;  /* Skip ecall instruction */
        break;

    case EXC_ECALL_S:
        /* Syscall from supervisor mode - should not happen normally */
        early_puts("[SYSCALL] Supervisor syscall ");
        early_puthex(tf->a7);
        early_puts("\n");
        tf->sepc += 4;
        break;

    case EXC_INST_PAGE_FAULT:
        do_page_fault(tf, FAULT_INST_FETCH);
        break;

    case EXC_LOAD_PAGE_FAULT:
        do_page_fault(tf, FAULT_LOAD);
        break;

    case EXC_STORE_PAGE_FAULT:
        do_page_fault(tf, FAULT_STORE);
        break;

    default:
        dump_trap_info(tf, "Unknown Exception");
        early_puts("  KERNEL PANIC: Unhandled exception\n");
        while (1) asm volatile("wfi");
        break;
    }
}

/* Get page fault count for debugging */
unsigned long get_page_fault_count(void)
{
    return page_fault_count;
}

/* Initialize trap handling */
void trap_init(void)
{
    early_puts("[TRAP] Initializing trap handling...\n");

    /* Set trap vector - all traps go to trap_vector */
    asm volatile ("csrw stvec, %0" :: "r"(&trap_vector));

    /* Enable supervisor external and timer interrupts in SIE */
    unsigned long sie = 0;
    sie |= (1UL << IRQ_S_EXT);    /* External interrupts */
    sie |= (1UL << IRQ_S_TIMER);  /* Timer interrupts */
    sie |= (1UL << IRQ_S_SOFT);   /* Software interrupts */
    asm volatile ("csrw sie, %0" :: "r"(sie));

    early_puts("[TRAP] Trap vector: ");
    early_puthex((unsigned long)&trap_vector);
    early_puts("\n[TRAP] SIE: ");
    early_puthex(sie);
    early_puts("\n[TRAP] Trap handling initialized\n");
}
