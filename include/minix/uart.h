/* UART Driver Interface */

#ifndef _MINIX_UART_H
#define _MINIX_UART_H

#include <types.h>

/* UART configuration */
#define UART_BUFFER_SIZE    256     /* Receive/transmit buffer size */

/* UART baud rates */
#define UART_BAUD_9600      9600
#define UART_BAUD_19200     19200
#define UART_BAUD_38400     38400
#define UART_BAUD_57600     57600
#define UART_BAUD_115200    115200

/* UART parity */
#define UART_PARITY_NONE    0
#define UART_PARITY_ODD     1
#define UART_PARITY_EVEN    2

/* UART stop bits */
#define UART_STOP_1         1
#define UART_STOP_2         2

/* UART data bits */
#define UART_DATA_5         5
#define UART_DATA_6         6
#define UART_DATA_7         7
#define UART_DATA_8         8

/* UART configuration structure */
typedef struct {
    u32 baud_rate;
    u8 data_bits;
    u8 parity;
    u8 stop_bits;
} uart_config_t;

/* UART statistics */
typedef struct {
    u64 tx_bytes;       /* Transmitted bytes */
    u64 rx_bytes;       /* Received bytes */
    u32 tx_errors;      /* Transmit errors */
    u32 rx_errors;      /* Receive errors */
    u32 overruns;       /* Buffer overruns */
} uart_stats_t;

/* Basic UART functions */
void console_init(void);
void uart_putc(char c);
char uart_getchar(void);
int uart_getc(void);
int uart_haschar(void);
void uart_puts(const char *s);

/* Advanced UART functions */
int uart_configure(const uart_config_t *config);
int uart_write(const void *buf, size_t len);
int uart_read(void *buf, size_t len);
void uart_flush_rx(void);
void uart_flush_tx(void);
int uart_get_stats(uart_stats_t *stats);

/* Interrupt-driven UART functions */
void uart_enable_rx_interrupt(void);
void uart_disable_rx_interrupt(void);
void uart_enable_tx_interrupt(void);
void uart_disable_tx_interrupt(void);

/* Board-specific functions */
void board_putchar(char c);
int board_getchar(void);

#endif /* _MINIX_UART_H */
