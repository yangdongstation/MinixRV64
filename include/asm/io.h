/* RISC-V I/O operations */

#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <types.h>

/* I/O memory barriers */
#define iob()   asm volatile ("fence iorw, iorw" ::: "memory")

/* Read I/O register */
static inline unsigned long readl(unsigned long addr)
{
    unsigned long val;
    asm volatile ("lw %0, 0(%1)" : "=r"(val) : "r"(addr));
    iob();
    return val;
}

/* Write I/O register */
static inline void writel(unsigned long val, unsigned long addr)
{
    asm volatile ("sw %0, 0(%1)" :: "r"(val), "r"(addr));
    iob();
}

/* Read 64-bit I/O register */
static inline unsigned long long readq(unsigned long addr)
{
    unsigned long long val;
    asm volatile ("ld %0, 0(%1)" : "=r"(val) : "r"(addr));
    iob();
    return val;
}

/* Write 64-bit I/O register */
static inline void writeq(unsigned long long val, unsigned long addr)
{
    asm volatile ("sd %0, 0(%1)" :: "r"(val), "r"(addr));
    iob();
}

/* Memory barriers */
#define mb()    asm volatile ("fence" ::: "memory")
#define rmb()   asm volatile ("fence r" ::: "memory")
#define wmb()   asm volatile ("fence w" ::: "memory")

#endif /* _ASM_IO_H */