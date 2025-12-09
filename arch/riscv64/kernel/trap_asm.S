/* RISC-V trap vector assembly */

.globl trap_vector
.align 2
trap_vector:
    /* Check if it's from S-mode or U-mode */
    csrr t0, sstatus
    andi t0, t0, 0x100   /* Check SPP bit */

    /* Save context and jump to C handler */
    /* For now, just jump to the C handler */
    tail do_trap