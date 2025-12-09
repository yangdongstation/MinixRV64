/* Minix RV64 Configuration */

#ifndef _MINIX_CONFIG_H
#define _MINIX_CONFIG_H

/* Platform identification */
#define BOARD_MILKV_DUO    1
#define CHIP_CV1800B       1

/* CPU configuration */
#define RISCV_64           1
#define RISCV_FREQ_MHZ     1000    /* 1GHz */
#define SMP_CPUS           1       /* Single core initially */

/* Memory configuration */
#define KERNEL_BASE_ADDR   0x80000000
#define KERNEL_SIZE        (64 * 1024 * 1024)  /* 64MB */
#define PAGE_SIZE          4096
#define PAGE_SHIFT         12

/* Clock and Timer */
#define TIMER_FREQ         1000000  /* 1MHz */
#define CLOCK_TICK_RATE    100      /* 100Hz system tick */

/* Debug */
#define DEBUG              1
#define EARLY_PRINTK       1       /* Use UART for early debug */

/* Minix specific */
#define NR_PROCS          32      /* Max processes */
#define NR_TASKS          8       /* Kernel tasks */
#define NR_VIRQS          64      /* Virtual IRQs */

#endif /* _MINIX_CONFIG_H */