/* HTIF (Host Target Interface) for QEMU debugging */

#include <minix/config.h>

/* HTIF device addresses */
#define HTIF_CONSOLE_DEV        1

/* Simple HTIF console write using memory mapped I/O */
void htif_putc(char c)
{
    /* HTIF uses a simple memory-mapped interface */
    volatile unsigned long *htif = (volatile unsigned long *)0x40008000;

    /* Write character to console */
    htif[0] = ((unsigned long)c << 56) | (HTIF_CONSOLE_DEV << 48) | 0x10000;

    /* Wait for completion */
    while (htif[0] & 0x1) {
        /* Wait */
    }
}

/* Initialize HTIF */
void htif_init(void)
{
    /* HTIF doesn't need explicit initialization */
}

/* Print string using HTIF */
void htif_puts(const char *s)
{
    while (*s) {
        htif_putc(*s++);
    }
}