/* Board-specific initialization */

#include <minix/config.h>
#include <minix/board.h>
#include <early_print.h>

/* Initialize board-specific hardware */
void board_init(void)
{
#if BOARD == BOARD_QEMU_VIRT
    /* QEMU virt board initialization */
    /* Most hardware is initialized by QEMU itself */
    /* Just need to ensure UART is ready (done in console_init) */

#elif BOARD == BOARD_MILKV_DUO
    /* CV1800B specific initialization */
    /* TODO: Add CV1800B specific initialization */

#endif
}

/* Initialize hardware interrupts */
void board_irq_init(void)
{
#if BOARD == BOARD_QEMU_VIRT
    /* QEMU uses PLIC and CLINT */
    /* TODO: Initialize PLIC */

#elif BOARD == BOARD_MILKV_DUO
    /* CV1800B interrupt controller */
    /* TODO: Initialize CV1800B INTC */

#endif
}

/* Initialize timer */
void board_timer_init(void)
{
#if BOARD == BOARD_QEMU_VIRT
    /* QEMU uses CLINT for timer */
    /* TODO: Setup timer interrupt */

#elif BOARD == BOARD_MILKV_DUO
    /* CV1800B timer initialization */
    /* TODO: Initialize hardware timer */

#endif
}