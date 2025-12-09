/* Context switch implementation for RISC-V 64-bit */

.section .text
.globl swtch

/* void swtch(struct context *old, struct context *new)
 *
 * Save current context to *old, restore context from *new
 *
 * struct context layout (offsets in bytes):
 *   0  - ra (return address)
 *   8  - sp (stack pointer)
 *   16 - s0
 *   24 - s1
 *   32 - s2
 *   40 - s3
 *   48 - s4
 *   56 - s5
 *   64 - s6
 *   72 - s7
 *   80 - s8
 *   88 - s9
 *   96 - s10
 *   104 - s11
 */

swtch:
    /* Save old context (a0 points to old context) */
    sd ra, 0(a0)        /* Save return address */
    sd sp, 8(a0)        /* Save stack pointer */
    sd s0, 16(a0)       /* Save callee-saved registers */
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    /* Restore new context (a1 points to new context) */
    ld ra, 0(a1)        /* Restore return address */
    ld sp, 8(a1)        /* Restore stack pointer */
    ld s0, 16(a1)       /* Restore callee-saved registers */
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    /* Return to new context */
    ret
