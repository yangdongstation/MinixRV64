/* 独立的UART读取测试 */
#include <stdint.h>

#define UART_BASE 0x10000000UL
#define UART_LSR  0x14
#define UART_RBR  0x00

#define UART_REG(base, off) (*((volatile uint32_t *)((base) + (off))))

void uart_putc_simple(char c) {
    UART_REG(UART_BASE, 0x00) = c;
    for (volatile int i = 0; i < 100; i++);
    if (c == '\n') {
        UART_REG(UART_BASE, 0x00) = '\r';
        for (volatile int i = 0; i < 100; i++);
    }
}

void uart_puts_simple(const char *s) {
    while (*s) {
        uart_putc_simple(*s++);
    }
}

void print_hex(uint32_t val) {
    const char *hex = "0123456789ABCDEF";
    uart_putc_simple(hex[(val >> 28) & 0xF]);
    uart_putc_simple(hex[(val >> 24) & 0xF]);
    uart_putc_simple(hex[(val >> 20) & 0xF]);
    uart_putc_simple(hex[(val >> 16) & 0xF]);
    uart_putc_simple(hex[(val >> 12) & 0xF]);
    uart_putc_simple(hex[(val >> 8) & 0xF]);
    uart_putc_simple(hex[(val >> 4) & 0xF]);
    uart_putc_simple(hex[val & 0xF]);
}

int main(void) {
    uart_puts_simple("\n\n=== UART Read Test ===\n");
    uart_puts_simple("Type a character...\n");

    int count = 0;
    while (count < 50) {  // 只尝试50次
        uint32_t lsr = UART_REG(UART_BASE, UART_LSR);

        if (lsr & 0x01) {  // Data Ready
            uint32_t ch = UART_REG(UART_BASE, UART_RBR) & 0xFF;
            uart_puts_simple("Got char: ");
            uart_putc_simple((char)ch);
            uart_puts_simple(" (0x");
            print_hex(ch);
            uart_puts_simple(")\n");

            if (ch == 'q' || ch == 'Q') break;
        }

        // 短延迟
        for (volatile int i = 0; i < 100000; i++);
        count++;
    }

    uart_puts_simple("Test done\n");

    // 挂起
    while (1) {
        asm volatile ("wfi");
    }
    return 0;
}
