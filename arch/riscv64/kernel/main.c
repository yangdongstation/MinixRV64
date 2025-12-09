/* Minix RV64 kernel main entry point */

#include <minix/config.h>
#include <minix/print.h>
#include <minix/board.h>
#include <asm/csr.h>

/* External symbols from linker script */
extern unsigned long __bss_start;
extern unsigned long __bss_end;
extern unsigned long __heap_start;
extern unsigned long __stack_end;

/* Forward declarations */
void console_init(void);
void trap_init(void);
void mm_init(void);
void sched_init(void);
void drivers_init(void);
void board_init(void);
void schedule(void);
void shell_init(void);
void shell_run(void);

/* Early debug functions */
void early_puts(const char *s);
void early_putchar(char c);
void early_puthex(unsigned long val);

void kinit(void)
{
    /* Initialize stack pointer for trap handling */
    asm volatile ("csrw sscratch, %0" :: "r"(&__stack_end));

    /* Clear BSS section */
    unsigned long *p = &__bss_start;
    while (p < &__bss_end) {
        *p++ = 0;
    }

    /* Board-specific initialization */
    board_init();

    /* Initialize kernel subsystems */
    trap_init();
    mm_init();
    sched_init();
    drivers_init();

    /* Enable interrupts */
    set_csr(CSR_SIE, SR_SIE | SR_SPIE);

    /* Print brief startup message */
    early_puts("\nMinix RV64 ready\n");

    /* Initialize and run shell */
    shell_init();
    shell_run();

    /* Should never reach here */
    while (1) {
        asm volatile ("wfi");
    }
}