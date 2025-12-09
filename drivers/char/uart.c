/* Universal UART driver supporting multiple boards */

#include <minix/config.h>
#include <minix/board.h>
#include <asm/io.h>
#include <types.h>

/* UART register offsets (standard 16550) */
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

/* Register access macros - use 32-bit access for QEMU NS16550A */
#define UART_REG(base, off)    (*((volatile unsigned int *)((base) + (off))))

/* Helper functions */
static inline void uart_write_reg(unsigned long base, unsigned long offset, unsigned int value)
{
    UART_REG(base, offset) = value;
}

static inline unsigned int uart_read_reg(unsigned long base, unsigned long offset)
{
    return UART_REG(base, offset);
}

/* Check if transmitter is ready */
static inline int uart_tx_ready(unsigned long base)
{
    return uart_read_reg(base, UART_LSR) & 0x20;  /* THRE bit */
}

/* Check if receiver has data */
static inline int uart_rx_ready(unsigned long base)
{
    return uart_read_reg(base, UART_LSR) & 0x01;  /* DR bit */
}

/* Initialize UART for specific board */
static void uart_init_board(unsigned long uart_base)
{
    unsigned long divisor;

    /* Disable interrupts */
    uart_write_reg(uart_base, UART_IER, 0x00);

    /* Set DLAB to access divisor */
    uart_write_reg(uart_base, UART_LCR, 0x80);

#if BOARD == BOARD_MILKV_DUO
    /* Calculate divisor for 115200 baud (CV1800B 25MHz clock) */
    divisor = 25000000 / (16 * 115200);
#elif BOARD == BOARD_QEMU_VIRT
    /* QEMU uses 115200 baud by default - but QEMU doesn't need divisor setting */
    divisor = 1;
#endif

    /* Set divisor */
    uart_write_reg(uart_base, UART_DLL, divisor & 0xFF);
    uart_write_reg(uart_base, UART_DLM, (divisor >> 8) & 0xFF);

    /* 8N1 configuration, clear DLAB */
    uart_write_reg(uart_base, UART_LCR, 0x03);

    /* Enable FIFO with reset */
    uart_write_reg(uart_base, UART_FCR, 0x01);  /* Enable FIFO only */

    /* Set RTS/DTR */
    uart_write_reg(uart_base, UART_MCR, 0x03);

    /* Small delay after init */
    volatile int delay;
    for (delay = 0; delay < 1000; delay++)
        ;
}

/* Initialize console */
void console_init(void)
{
    /* Initialize the board-specific UART */
    uart_init_board(BOARD_UART_BASE);
}

/* Put character */
void uart_putc(char c)
{
    volatile int delay;

    /* Write directly to UART */
    uart_write_reg(BOARD_UART_BASE, UART_THR, c);

    /* Small delay */
    for (delay = 0; delay < 100; delay++)
        ;

    if (c == '\n') {
        uart_write_reg(BOARD_UART_BASE, UART_THR, '\r');
        for (delay = 0; delay < 100; delay++)
            ;
    }
}

/* Check if character is available */
int uart_haschar(void)
{
    /* Try direct read - if LSR DR bit is set, data is available */
    volatile unsigned int lsr = uart_read_reg(BOARD_UART_BASE, UART_LSR);
    return lsr & 0x01;
}

/* Get character (blocking) */
char uart_getchar(void)
{
    volatile int timeout;

    /* Poll with timeout to avoid infinite loop */
    while (1) {
        volatile unsigned int lsr = uart_read_reg(BOARD_UART_BASE, UART_LSR);
        if (lsr & 0x01) {
            /* Data ready */
            return uart_read_reg(BOARD_UART_BASE, UART_RBR) & 0xFF;
        }

        /* Small delay before retry */
        for (timeout = 0; timeout < 10; timeout++)
            ;
    }
}

/* Get character (non-blocking) */
int uart_getc(void)
{
    volatile unsigned int lsr = uart_read_reg(BOARD_UART_BASE, UART_LSR);

    if (!(lsr & 0x01))
        return -1;

    return uart_read_reg(BOARD_UART_BASE, UART_RBR) & 0xFF;
}

/* Print string */
void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

/* Board-specific console functions */
void board_putchar(char c)
{
    uart_putc(c);
}

int board_getchar(void)
{
    return uart_getc();
}