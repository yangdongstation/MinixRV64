/* Simple printk implementation */

#include <minix/config.h>
#include <minix/print.h>
#include <stdarg.h>

/* External functions from UART driver */
extern void uart_putc(char c);
extern void uart_puts(const char *s);

/* Convert integer to string */
static void itoa(int num, char *str, int base)
{
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    /* Convert to string in reverse order */
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    /* Reverse the string */
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/* Simple printf implementation */
void printf(const char *fmt, ...)
{
    va_list args;
    char buffer[32];
    const char *p;

    va_start(args, fmt);

    for (p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            uart_putc(*p);
            continue;
        }

        p++;

        switch (*p) {
        case 'd':
            itoa(va_arg(args, int), buffer, 10);
            uart_puts(buffer);
            break;

        case 'x':
            uart_puts("0x");
            itoa(va_arg(args, int), buffer, 16);
            uart_puts(buffer);
            break;

        case 's':
            uart_puts(va_arg(args, const char *));
            break;

        case 'c':
            uart_putc(va_arg(args, int));
            break;

        case '%':
            uart_putc('%');
            break;

        default:
            uart_putc('%');
            uart_putc(*p);
            break;
        }
    }

    va_end(args);
}

/* printk with levels */
void printk(const char *fmt, ...)
{
    /* Skip print level for now */
    while (*fmt && *fmt != ' ') {
        fmt++;
    }
    if (*fmt == ' ') fmt++;

    va_list args;
    va_start(args, fmt);

    /* Use printf for now */
    printf(fmt);

    va_end(args);
}

/* Simple puts */
void puts(const char *s)
{
    uart_puts(s);
    uart_putc('\n');
}