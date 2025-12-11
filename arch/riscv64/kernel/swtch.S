/* Context switch implementation for RISC-V 64-bit
 *
 * MinixRV64 Donz Build - Enhanced for Stage 2
 */

.section .text
.globl swtch
.globl ret_from_fork
.globl ret_to_user

/* ============================================
 * Trap Frame (pt_regs) offsets
 * Must match struct trapframe in task.h
 * ============================================ */
#define PT_RA       0
#define PT_SP       8
#define PT_GP       16
#define PT_TP       24
#define PT_T0       32
#define PT_T1       40
#define PT_T2       48
#define PT_S0       56
#define PT_S1       64
#define PT_A0       72
#define PT_A1       80
#define PT_A2       88
#define PT_A3       96
#define PT_A4       104
#define PT_A5       112
#define PT_A6       120
#define PT_A7       128
#define PT_S2       136
#define PT_S3       144
#define PT_S4       152
#define PT_S5       160
#define PT_S6       168
#define PT_S7       176
#define PT_S8       184
#define PT_S9       192
#define PT_S10      200
#define PT_S11      208
#define PT_T3       216
#define PT_T4       224
#define PT_T5       232
#define PT_T6       240
#define PT_SEPC     248
#define PT_SSTATUS  256
#define PT_SCAUSE   264
#define PT_STVAL    272
#define PT_SIZE     288

/* ============================================
 * Context Switch
 *
 * void swtch(struct context *old, struct context *new)
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
 * ============================================ */

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

/* ============================================
 * Return from Fork
 *
 * Entry point for new process after fork.
 * Called after swtch() returns to child process.
 * sp points to child's trapframe.
 * ============================================ */

ret_from_fork:
    /* Call schedule_tail() to finish scheduler work */
    call schedule_tail

    /* Fall through to ret_to_user */

/* ============================================
 * Return to User Mode
 *
 * Restore all registers from trapframe and sret.
 * sp must point to struct trapframe.
 * ============================================ */

ret_to_user:
    /* sp points to trapframe */

    /* Restore sstatus (need to do this before restoring other regs) */
    ld t0, PT_SSTATUS(sp)
    csrw sstatus, t0

    /* Restore sepc */
    ld t0, PT_SEPC(sp)
    csrw sepc, t0

    /* Restore general purpose registers */
    ld ra,  PT_RA(sp)
    /* Skip sp for now */
    ld gp,  PT_GP(sp)
    ld tp,  PT_TP(sp)
    ld t0,  PT_T0(sp)
    ld t1,  PT_T1(sp)
    ld t2,  PT_T2(sp)
    ld s0,  PT_S0(sp)
    ld s1,  PT_S1(sp)
    ld a0,  PT_A0(sp)
    ld a1,  PT_A1(sp)
    ld a2,  PT_A2(sp)
    ld a3,  PT_A3(sp)
    ld a4,  PT_A4(sp)
    ld a5,  PT_A5(sp)
    ld a6,  PT_A6(sp)
    ld a7,  PT_A7(sp)
    ld s2,  PT_S2(sp)
    ld s3,  PT_S3(sp)
    ld s4,  PT_S4(sp)
    ld s5,  PT_S5(sp)
    ld s6,  PT_S6(sp)
    ld s7,  PT_S7(sp)
    ld s8,  PT_S8(sp)
    ld s9,  PT_S9(sp)
    ld s10, PT_S10(sp)
    ld s11, PT_S11(sp)
    ld t3,  PT_T3(sp)
    ld t4,  PT_T4(sp)
    ld t5,  PT_T5(sp)
    ld t6,  PT_T6(sp)

    /* Finally restore sp */
    ld sp, PT_SP(sp)

    /* Return to user mode (or S-mode depending on sstatus.SPP) */
    sret
