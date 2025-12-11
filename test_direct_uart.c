/* 最简单的UART测试 - 直接访问硬件 */

#define UART_BASE 0x10000000UL
#define UART_LSR  0x14
#define UART_RBR  0x00
#define UART_THR  0x00

#define UART_REG(base, off) (*((volatile unsigned int *)((base) + (off))))

void putchar_simple(char c) {
    // 等待发送器就绪
    while (!(UART_REG(UART_BASE, UART_LSR) & 0x20));
    UART_REG(UART_BASE, UART_THR) = c;
}

void puts_simple(const char *s) {
    while (*s) {
        putchar_simple(*s++);
        if (*(s-1) == '\n') putchar_simple('\r');
    }
}

void print_hex(unsigned int val) {
    const char *hex = "0123456789ABCDEF";
    putchar_simple('0');
    putchar_simple('x');
    putchar_simple(hex[(val >> 28) & 0xF]);
    putchar_simple(hex[(val >> 24) & 0xF]);
    putchar_simple(hex[(val >> 20) & 0xF]);
    putchar_simple(hex[(val >> 16) & 0xF]);
    putchar_simple(hex[(val >> 12) & 0xF]);
    putchar_simple(hex[(val >> 8) & 0xF]);
    putchar_simple(hex[(val >> 4) & 0xF]);
    putchar_simple(hex[val & 0xF]);
}

int main(void) {
    puts_simple("\n\n=== Direct UART Test ===\n");
    puts_simple("Reading LSR register continuously...\n");
    puts_simple("Type something and watch LSR change!\n");
    puts_simple("Press 'q' to quit\n\n");

    int iterations = 0;
    unsigned int last_lsr = 0;

    while (iterations < 1000) {  // 1000次尝试
        unsigned int lsr = UART_REG(UART_BASE, UART_LSR);

        // 只在LSR改变时打印
        if (lsr != last_lsr) {
            puts_simple("LSR changed: ");
            print_hex(lsr);
            puts_simple("\n");
            last_lsr = lsr;
        }

        // 检查数据就绪位
        if (lsr & 0x01) {
            unsigned int ch = UART_REG(UART_BASE, UART_RBR) & 0xFF;
            puts_simple("*** GOT CHARACTER: ");
            putchar_simple((char)ch);
            puts_simple(" (");
            print_hex(ch);
            puts_simple(") ***\n");

            if (ch == 'q' || ch == 'Q') {
                puts_simple("Quit requested\n");
                break;
            }
        }

        // 小延迟
        for (volatile int i = 0; i < 50000; i++);
        iterations++;
    }

    puts_simple("\nTest complete. Final LSR: ");
    print_hex(UART_REG(UART_BASE, UART_LSR));
    puts_simple("\n");

    // 挂起
    while (1) {
        asm volatile ("wfi");
    }

    return 0;
}
