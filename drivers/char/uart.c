/* Universal UART driver supporting multiple boards */

#include <minix/config.h>
#include <minix/board.h>
#include <minix/uart.h>
#include <asm/io.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Circular buffer structure */
typedef struct {
    char data[UART_BUFFER_SIZE];
    volatile u32 head;
    volatile u32 tail;
    volatile u32 count;
} uart_buffer_t;

/* UART state */
static struct {
    uart_buffer_t rx_buffer;
    uart_buffer_t tx_buffer;
    uart_stats_t stats;
    uart_config_t config;
    volatile int tx_busy;
} uart_state;

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

/* Initialize buffers */
static void uart_buffer_init(void)
{
    uart_state.rx_buffer.head = 0;
    uart_state.rx_buffer.tail = 0;
    uart_state.rx_buffer.count = 0;

    uart_state.tx_buffer.head = 0;
    uart_state.tx_buffer.tail = 0;
    uart_state.tx_buffer.count = 0;

    uart_state.stats.tx_bytes = 0;
    uart_state.stats.rx_bytes = 0;
    uart_state.stats.tx_errors = 0;
    uart_state.stats.rx_errors = 0;
    uart_state.stats.overruns = 0;

    uart_state.tx_busy = 0;

    uart_state.config.baud_rate = UART_BAUD_115200;
    uart_state.config.data_bits = UART_DATA_8;
    uart_state.config.parity = UART_PARITY_NONE;
    uart_state.config.stop_bits = UART_STOP_1;
}

/* Buffer operations */
static int buffer_put(uart_buffer_t *buf, char c)
{
    if (buf->count >= UART_BUFFER_SIZE) {
        uart_state.stats.overruns++;
        return -1;  /* Buffer full */
    }

    buf->data[buf->head] = c;
    buf->head = (buf->head + 1) % UART_BUFFER_SIZE;
    buf->count++;
    return 0;
}

static int buffer_get(uart_buffer_t *buf, char *c)
{
    if (buf->count == 0) {
        return -1;  /* Buffer empty */
    }

    *c = buf->data[buf->tail];
    buf->tail = (buf->tail + 1) % UART_BUFFER_SIZE;
    buf->count--;
    return 0;
}

static int __attribute__((unused)) buffer_peek(uart_buffer_t *buf)
{
    if (buf->count == 0) {
        return -1;
    }
    return buf->data[buf->tail];
}

/* Initialize console */
void console_init(void)
{
    /* Initialize buffers */
    uart_buffer_init();

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
    volatile unsigned int lsr;
    lsr = uart_read_reg(BOARD_UART_BASE, UART_LSR);
    return (lsr & 0x01) ? 1 : 0;
}

/* Get character (blocking) */
char uart_getchar(void)
{
    volatile unsigned int lsr;
    char ch;

    /* Check LSR once per call - no tight loop */
    lsr = uart_read_reg(BOARD_UART_BASE, UART_LSR);
    if (lsr & 0x01) {
        /* Data ready - read it */
        ch = uart_read_reg(BOARD_UART_BASE, UART_RBR) & 0xFF;
        return ch;
    }
    /* No data available - return null character */
    return '\0';
}

/* Get character (non-blocking) */
/* External debug function */
extern void early_puts(const char *s);

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

/* Configure UART */
int uart_configure(const uart_config_t *config)
{
    unsigned long divisor;
    u8 lcr = 0;

    if (config == NULL) {
        return -1;
    }

    /* Save configuration */
    uart_state.config = *config;

    /* Disable interrupts during configuration */
    uart_write_reg(BOARD_UART_BASE, UART_IER, 0x00);

    /* Set DLAB to access divisor */
    uart_write_reg(BOARD_UART_BASE, UART_LCR, 0x80);

    /* Calculate and set divisor based on baud rate */
#if BOARD == BOARD_MILKV_DUO
    divisor = 25000000 / (16 * config->baud_rate);
#elif BOARD == BOARD_QEMU_VIRT
    divisor = 1;  /* QEMU handles baud rate automatically */
#endif

    uart_write_reg(BOARD_UART_BASE, UART_DLL, divisor & 0xFF);
    uart_write_reg(BOARD_UART_BASE, UART_DLM, (divisor >> 8) & 0xFF);

    /* Configure data bits */
    switch (config->data_bits) {
        case UART_DATA_5: lcr |= 0x00; break;
        case UART_DATA_6: lcr |= 0x01; break;
        case UART_DATA_7: lcr |= 0x02; break;
        case UART_DATA_8: lcr |= 0x03; break;
        default: return -1;
    }

    /* Configure parity */
    switch (config->parity) {
        case UART_PARITY_NONE: break;
        case UART_PARITY_ODD:  lcr |= 0x08; break;
        case UART_PARITY_EVEN: lcr |= 0x18; break;
        default: return -1;
    }

    /* Configure stop bits */
    if (config->stop_bits == UART_STOP_2) {
        lcr |= 0x04;
    }

    /* Write LCR and clear DLAB */
    uart_write_reg(BOARD_UART_BASE, UART_LCR, lcr);

    return 0;
}

/* Write data to UART */
int uart_write(const void *buf, size_t len)
{
    const char *data = (const char *)buf;
    size_t i;

    if (buf == NULL || len == 0) {
        return 0;
    }

    for (i = 0; i < len; i++) {
        uart_putc(data[i]);
        uart_state.stats.tx_bytes++;
    }

    return (int)len;
}

/* Read data from UART */
int uart_read(void *buf, size_t len)
{
    char *data = (char *)buf;
    size_t i = 0;
    int c;

    if (buf == NULL || len == 0) {
        return 0;
    }

    while (i < len) {
        c = uart_getc();
        if (c < 0) {
            break;  /* No more data */
        }
        data[i++] = (char)c;
        uart_state.stats.rx_bytes++;
    }

    return (int)i;
}

/* Flush receive buffer */
void uart_flush_rx(void)
{
    uart_state.rx_buffer.head = 0;
    uart_state.rx_buffer.tail = 0;
    uart_state.rx_buffer.count = 0;

    /* Drain hardware FIFO */
    while (uart_rx_ready(BOARD_UART_BASE)) {
        uart_read_reg(BOARD_UART_BASE, UART_RBR);
    }
}

/* Flush transmit buffer */
void uart_flush_tx(void)
{
    uart_state.tx_buffer.head = 0;
    uart_state.tx_buffer.tail = 0;
    uart_state.tx_buffer.count = 0;
}

/* Get UART statistics */
int uart_get_stats(uart_stats_t *stats)
{
    if (stats == NULL) {
        return -1;
    }

    *stats = uart_state.stats;
    return 0;
}

/* Interrupt control functions */
void uart_enable_rx_interrupt(void)
{
    u32 ier = uart_read_reg(BOARD_UART_BASE, UART_IER);
    ier |= 0x01;  /* Enable Received Data Available interrupt */
    uart_write_reg(BOARD_UART_BASE, UART_IER, ier);
}

void uart_disable_rx_interrupt(void)
{
    u32 ier = uart_read_reg(BOARD_UART_BASE, UART_IER);
    ier &= ~0x01;  /* Disable Received Data Available interrupt */
    uart_write_reg(BOARD_UART_BASE, UART_IER, ier);
}

void uart_enable_tx_interrupt(void)
{
    u32 ier = uart_read_reg(BOARD_UART_BASE, UART_IER);
    ier |= 0x02;  /* Enable Transmitter Holding Register Empty interrupt */
    uart_write_reg(BOARD_UART_BASE, UART_IER, ier);
}

void uart_disable_tx_interrupt(void)
{
    u32 ier = uart_read_reg(BOARD_UART_BASE, UART_IER);
    ier &= ~0x02;  /* Disable Transmitter Holding Register Empty interrupt */
    uart_write_reg(BOARD_UART_BASE, UART_IER, ier);
}

/* UART interrupt handler (to be called from trap handler) */
void uart_interrupt_handler(void)
{
    u32 iir;
    char c;

    iir = uart_read_reg(BOARD_UART_BASE, UART_IIR);

    /* Check interrupt type */
    if ((iir & 0x0F) == 0x04) {
        /* Received Data Available */
        while (uart_rx_ready(BOARD_UART_BASE)) {
            c = uart_read_reg(BOARD_UART_BASE, UART_RBR) & 0xFF;
            buffer_put(&uart_state.rx_buffer, c);
            uart_state.stats.rx_bytes++;
        }
    } else if ((iir & 0x0F) == 0x02) {
        /* Transmitter Holding Register Empty */
        if (buffer_get(&uart_state.tx_buffer, &c) == 0) {
            uart_write_reg(BOARD_UART_BASE, UART_THR, c);
            uart_state.stats.tx_bytes++;
        } else {
            /* No more data to send, disable TX interrupt */
            uart_disable_tx_interrupt();
            uart_state.tx_busy = 0;
        }
    }
}