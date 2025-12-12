/* Minimal UART test for QEMU virt */

.section .text
.globl _start

_start:
    /* Write 'H' directly to UART */
    li t0, 0x10000000    /* UART base */
    li t1, 0x48          /* 'H' */
    sw t1, 0(t0)

    /* Write 'i' */
    li t1, 0x69          /* 'i' */
    sw t1, 0(t0)

    /* Write newline */
    li t1, 0x0A          /* '\n' */
    sw t1, 0(t0)

    /* Infinite loop */
1:
    wfi
    j 1b
