/* MilkV Duo CV1800B board definitions */

#ifndef _BOARD_CV1800B_H
#define _BOARD_CV1800B_H

/* Physical memory map */
#define CV1800B_DRAM_BASE       0x80000000UL
#define CV1800B_DRAM_SIZE       (512 * 1024 * 1024)  /* 512MB */

/* I/O register base addresses */
#define CV1800B_IO_BASE         0x07000000UL
#define CV1800B_CTRL_BASE       0x03000000UL
#define CV1800B_CLK_BASE        0x02000000UL
#define CV1800B_PINMUX_BASE     0x03001000UL
#define CV1800B_GPIO_BASE       0x03010000UL
#define CV1800B_UART0_BASE      0x04140000UL
#define CV1800B_UART1_BASE      0x04141000UL
#define CV1800B_UART2_BASE      0x04142000UL
#define CV1800B_UART3_BASE      0x04143000UL
#define CV1800B_UART4_BASE      0x04144000UL

/* UART register offsets */
#define UART_RBR                0x00    /* Receiver Buffer Register */
#define UART_THR                0x00    /* Transmitter Holding Register */
#define UART_IER                0x04    /* Interrupt Enable Register */
#define UART_IIR                0x08    /* Interrupt Identification Register */
#define UART_FCR                0x08    /* FIFO Control Register */
#define UART_LCR                0x0C    /* Line Control Register */
#define UART_MCR                0x10    /* Modem Control Register */
#define UART_LSR                0x14    /* Line Status Register */
#define UART_MSR                0x18    /* Modem Status Register */
#define UART_SCR                0x1C    /* Scratch Register */
#define UART_DLL                0x00    /* Divisor Latch LSB */
#define UART_DLM                0x04    /* Divisor Latch MSB */

/* Interrupt controller */
#define CV1800B_INTC_BASE       0x03800000UL
#define INTC_BASE               CV1800B_INTC_BASE

/* Timer */
#define CV1800B_TIMER_BASE      0x02020000UL
#define TIMER_BASE              CV1800B_TIMER_BASE

/* Clock control */
#define CV1800B_CLK_CONTROL     0x74    /* System clock control register */
#define CV1800B_CLK_UART        0xB8    /* UART clock control */

/* Pinmux for UART */
#define PINMUX_UART0_TX         0x2C    /* UART0 TX pinmux */
#define PINMUX_UART0_RX         0x30    /* UART0 RX pinmux */

/* Device tree compatible strings */
#define CV1800B_COMPATIBLE      "sophgo,cv1800b"
#define MILKV_DUO_COMPATIBLE    "milkv,duo"

/* Board identification */
#define BOARD_NAME              "MilkV Duo"
#define BOARD_REV               "1.0"

#endif /* _BOARD_CV1800B_H */