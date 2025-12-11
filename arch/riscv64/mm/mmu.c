/* RISC-V Memory Management Unit initialization */

#include <minix/config.h>
#include <asm/csr.h>
#include <minix/print.h>
#include <minix/board.h>
#include <early_print.h>

/* External functions */
void page_init(void);
void kmem_init(void);
int pgtable_init(void);
void enable_mmu(void);
void get_mem_info(unsigned long *total, unsigned long *free);
void vmalloc_init(void);

/* Initialize MMU */
void mm_init(void)
{
    early_puts("\n=== Memory Management Initialization ===\n");

    /* Initialize buddy allocator for physical pages */
    page_init();

    /* Initialize slab allocator for kernel objects */
    kmem_init();

    /* Initialize page tables and enable MMU */
    if (pgtable_init() == 0) {
        enable_mmu();
        early_puts("✓ MMU enabled with SV39 paging\n");
    } else {
        early_puts("✗ Page table initialization failed\n");
    }

    /* Initialize vmalloc subsystem */
    vmalloc_init();

    early_puts("=== Memory Management Ready ===\n\n");
}
