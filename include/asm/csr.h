/* RISC-V Control and Status Registers */

#ifndef _ASM_CSR_H
#define _ASM_CSR_H

/* CSR numbers */
#define CSR_STATUS     0x100
#define CSR_IE         0x104
#define CSR_TVEC       0x105
#define CSR_TSCRATCH   0x140
#define CSR_EPC        0x141
#define CSR_CAUSE      0x142
#define CSR_TVAL       0x143
#define CSR_IP         0x144

/* Machine mode CSRs */
#define CSR_MSTATUS    0x300
#define CSR_MISA       0x301
#define CSR_MEDELEG    0x302
#define CSR_MIDELEG    0x303
#define CSR_MIE        0x304
#define CSR_MTVEC      0x305
#define CSR_MSCRATCH   0x340
#define CSR_MEPC       0x341
#define CSR_MCAUSE     0x342
#define CSR_MTVAL      0x343
#define CSR_MIP        0x344

/* PMP configuration */
#define CSR_PMPCFG0    0x3a0
#define CSR_PMPADDR0   0x3b0

/* Machine-level counters */
#define CSR_MCYCLE     0xb00
#define CSR_MTIME      0xb01
#define CSR_MINSTRET   0xb02

/* Supervisor mode CSRs */
#define CSR_SSTATUS    0x100
#define CSR_SIE        0x104
#define CSR_STVEC      0x105
#define CSR_SSCRATCH   0x140
#define CSR_SEPC       0x141
#define CSR_SCAUSE     0x142
#define CSR_STVAL      0x143
#define CSR_SIP        0x144

/* SATP register (Supervisor Address Translation and Protection) */
#define CSR_SATP       0x180

/* STATUS field bits */
#define SR_SIE         0x00000002
#define SR_MIE         0x00000008
#define SR_SPIE        0x00000020
#define SR_SPP         0x00000100
#define SR_MPP         0x00001800
#define SR_FS          0x00006000
#define SR_XS          0x00018000
#define SR_SUM         0x00040000
#define SR_MXR         0x00080000
#define SR_SD          0x80000000

/* SSTATUS bits */
#define SSTATUS_SIE    0x00000002
#define SSTATUS_SPIE   0x00000020
#define SSTATUS_SPP    0x00000100
#define SSTATUS_FS     0x00006000
#define SSTATUS_XS     0x00018000
#define SSTATUS_SUM    0x00040000
#define SSTATUS_MXR    0x00080000
#define SSTATUS_SD     0x80000000

/* Interrupt causes */
#define IRQ_S_SOFT     1
#define IRQ_S_TIMER    5
#define IRQ_S_EXT      9
#define IRQ_M_SOFT     3
#define IRQ_M_TIMER    7
#define IRQ_M_EXT      11

/* Exception causes */
#define EXC_INST_MISALIGNED    0
#define EXC_INST_ACCESS        1
#define EXC_INST_ILLEGAL       2
#define EXC_BREAKPOINT         3
#define EXC_LOAD_MISALIGNED    4
#define EXC_LOAD_ACCESS        5
#define EXC_STORE_MISALIGNED   6
#define EXC_STORE_ACCESS       7
#define EXC_ECALL_U            8
#define EXC_ECALL_S            9
#define EXC_ECALL_M            11
#define EXC_INST_PAGE_FAULT    12
#define EXC_LOAD_PAGE_FAULT    13
#define EXC_STORE_PAGE_FAULT   15

/* SATP modes */
#define SATP_MODE_BARE  0
#define SATP_MODE_SV39  8
#define SATP_MODE_SV48  9
#define SATP_MODE_SV57  10
#define SATP_MODE_SV64  11

#define SATP_ASID_SHIFT    44
#define SATP_PPN_SHIFT     12

/* CSR read/write macros */
#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define write_csr(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

#define set_csr(reg, bit) ({ unsigned long __tmp; \
  __tmp = read_csr(reg) | (bit); \
  write_csr(reg, __tmp); })

#define clear_csr(reg, bit) ({ unsigned long __tmp; \
  __tmp = read_csr(reg) & ~(bit); \
  write_csr(reg, __tmp); })

#endif /* _ASM_CSR_H */