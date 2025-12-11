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

/* Initialize MMU */
void mm_init(void)
{
    /* Initialize page allocator */
    page_init();

    /* Initialize slab allocator */
    kmem_init();

    /* Initialize page tables */
    if (pgtable_init() == 0) {
        /* TEMPORARILY DISABLE MMU TO TEST INPUT */
        /* enable_mmu(); */
        early_puts("✓ MMU ready (DISABLED)\n");
    } else {
        early_puts("! Page table init failed\n");
    }
}


/* Page table operations */

void flush_tlb_range(unsigned long start, unsigned long size)
{
    /* Suppress unused parameter warning */
    (void)size;
    asm volatile ("sfence.vma %0" :: "r"(start) : "memory");
}