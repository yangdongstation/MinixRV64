/* Minix RV64 startup code for MilkV Duo CV1800B */

#include <asm/csr.h>

.section .text.init
.globl _start
.globl early_puthex
_start:
    /* Direct UART test - write 'X' to console */
    li t0, 0x10000000    /* UART base */
    li t1, 0x58          /* 'X' */
    sw t1, 0(t0)

    /* Initialize UART first */
    call early_uart_init

    /* Disable all interrupts */
    csrw CSR_IE, x0
    csrw CSR_IP, x0

    /* Configure PMP to allow all memory access (must do in M-mode) */
    li t0, 0xf
    csrw CSR_PMPCFG0, t0
    li t0, -1
    csrw CSR_PMPADDR0, t0

    /* Setup stack */
    la sp, __stack_end

    /* Prepare to transition from M-mode to S-mode */
    /* Set MSTATUS.MPP to S-mode (01) */
    csrr t0, CSR_MSTATUS
    li t1, (3 << 11)       /* MPP mask at bits [12:11] */
    not t1, t1             /* Invert to clear bits */
    and t0, t0, t1         /* Clear MPP bits */
    li t1, (1 << 11)       /* MPP = 01 (S-mode) */
    or t0, t0, t1          /* Set MPP to S-mode */
    csrw CSR_MSTATUS, t0

    /* Set MEPC to kinit address (return address for mret) */
    la t0, kinit
    csrw CSR_MEPC, t0

    /* Delegate exceptions to S-mode */
    li t0, 0xffff
    csrw CSR_MEDELEG, t0   /* Delegate all exceptions */
    li t0, 0xffff
    csrw CSR_MIDELEG, t0   /* Delegate all interrupts */

    /* Return from M-mode to S-mode, jumping to kinit */
    mret

    /* Should never reach here */
1:  la a0, error_msg
    call early_puts
    wfi
    j 1b

/* Helper: print hex */
early_puthex:
    addi sp, sp, -32
    sd ra, 0(sp)
    sd t0, 8(sp)
    sd t1, 16(sp)
    sd t2, 24(sp)

    mv t0, a0           /* Save value to t0 */
    li t1, 60           /* Start at bit 60 */
1:
    srl t2, t0, t1      /* Shift right to get nibble */
    andi t2, t2, 0xf    /* Mask to 4 bits */
    la t3, hex_chars    /* Load hex_chars address */
    add t3, t3, t2      /* Add offset */
    lbu a0, 0(t3)       /* Load character */
    call early_putchar  /* Print it */
    addi t1, t1, -4     /* Next nibble */
    bgez t1, 1b         /* Continue if t1 >= 0 */

    ld t2, 24(sp)
    ld t1, 16(sp)
    ld t0, 8(sp)
    ld ra, 0(sp)
    addi sp, sp, 32
    ret

/* Data section */
.section .rodata
boot_msg: .asciz "[BOOT] Minix RV64 starting...\n"
stack_msg: .asciz "[BOOT] Stack at: 0x"
entering_c_msg: .asciz "[BOOT] Entering C code...\n"
error_msg: .asciz "[ERROR] Should not reach here!\n"
newline: .asciz "\n"
hex_chars: .asciz "0123456789ABCDEF"

/* Include early console functions */
.include "arch/riscv64/kernel/early_console.S"