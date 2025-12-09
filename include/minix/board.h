/* Board configuration system */

#ifndef _MINIX_BOARD_H
#define _MINIX_BOARD_H

/* Board selection */
#define BOARD_MILKV_DUO      1
#define BOARD_QEMU_VIRT      2

/* Default board for testing */
#ifndef BOARD
#define BOARD                BOARD_QEMU_VIRT
#endif

/* Include appropriate board header */
#if BOARD == BOARD_MILKV_DUO
#include <board/cv1800b.h>
#elif BOARD == BOARD_QEMU_VIRT
#include <board/qemu-virt.h>
#else
#error "Unsupported board type"
#endif

/* Board-specific function prototypes */
void board_init(void);
void board_putchar(char c);
int board_getchar(void);
void board_timer_init(void);
void board_irq_init(void);

/* Common board functions */
#if BOARD == BOARD_MILKV_DUO
#define BOARD_UART_BASE      CV1800B_UART0_BASE
#elif BOARD == BOARD_QEMU_VIRT
#define BOARD_UART_BASE      QEMU_VIRT_UART0_BASE
#endif

#endif /* _MINIX_BOARD_H */