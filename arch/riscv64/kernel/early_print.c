/* Early print functions for debugging */

/* Early character output using multiple methods */
void early_putc(char c)
{
    /* Method 1: Try HTIF (QEMU specific) */
    volatile unsigned long *htif = (volatile unsigned long *)0x40008000;
    htif[0] = ((unsigned long)c << 56) | 0x10000;

    /* Method 2: Try SBI console_putchar (if available) */
    /* This would require SBI support */
    asm volatile (
        "li a7, 1\n"      /* SBI console_putchar */
        "mv a6, %0\n"      /* character */
        "ecall\n"
        :
        : "r"((unsigned long)c)
        : "a7", "a6"
    );
}

/* Print a simple string */
void early_puts(const char *s)
{
    while (*s) {
        early_putc(*s++);
    }
}

/* Print a number in hex */
void early_puthex(unsigned long num)
{
    int i;
    early_puts("0x");
    for (i = 60; i >= 0; i -= 4) {
        unsigned long digit = (num >> i) & 0xF;
        early_putc("0123456789ABCDEF"[digit]);
    }
}