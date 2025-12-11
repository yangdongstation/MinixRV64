/* RISC-V SV39 page table management */

#include <minix/config.h>
#include <types.h>
#include <asm/csr.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* External TLB flush function */
extern void flush_tlb_page(unsigned long addr);

/* SV39 page table constants */
#define PGDIR_SHIFT      30
#define PMD_SHIFT        21
#define PTE_SHIFT        12

/* Page table entries per level */
#define PTRS_PER_PTE     512

/* Page table entry flags */
#define PTE_V            (1UL << 0)    /* Valid */
#define PTE_R            (1UL << 1)    /* Read */
#define PTE_W            (1UL << 2)    /* Write */
#define PTE_X            (1UL << 3)    /* Execute */
#define PTE_U            (1UL << 4)    /* User */
#define PTE_G            (1UL << 5)    /* Global */
#define PTE_A            (1UL << 6)    /* Accessed */
#define PTE_D            (1UL << 7)    /* Dirty */
#define PTE_PPN_SHIFT    10

/* Page table entry structure */
typedef unsigned long pte_t;

/* Static page directory - 4KB aligned */
static pte_t root_pgtable[PTRS_PER_PTE] __attribute__((aligned(4096)));

/* Satp register value */
static unsigned long satp_value = 0;

/* Page allocation functions */
extern unsigned long alloc_page(void);
extern void free_page(unsigned long addr);
extern unsigned long phys_to_virt(unsigned long addr);
extern unsigned long virt_to_phys(unsigned long addr);

/* Static second-level page table for UART region */
static pte_t uart_pgtable[PTRS_PER_PTE] __attribute__((aligned(4096)));

/* Initialize page table */
int pgtable_init(void)
{
    extern void early_puts(const char *);

    /* Clear root page directory */
    for (int i = 0; i < PTRS_PER_PTE; i++) {
        root_pgtable[i] = 0;
    }

    /* Clear UART page table */
    for (int i = 0; i < PTRS_PER_PTE; i++) {
        uart_pgtable[i] = 0;
    }

    early_puts("[MMU] Setting up page tables...\n");

    /* Map 0x00000000-0x0FFFFFFF using gigapage (for CLINT, PLIC, etc) */
    unsigned long pa = 0;
    root_pgtable[0] = (pa >> 12) << PTE_PPN_SHIFT;
    root_pgtable[0] |= PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;

    /* Map 0x10000000-0x1FFFFFFF (UART region) using 2-level page table with 4KB pages */
    unsigned long uart_pt_pa = virt_to_phys((unsigned long)uart_pgtable);
    root_pgtable[0x10000000UL >> PGDIR_SHIFT] = (uart_pt_pa >> 12) << PTE_PPN_SHIFT;
    root_pgtable[0x10000000UL >> PGDIR_SHIFT] |= PTE_V;  /* Non-leaf, only V bit */

    /* Map 4KB pages for UART region (0x10000000-0x10001000) */
    /* At second level, we map 2MB regions, each entry covers 4KB */
    /* Index for 0x10000000: bits [20:12] = 0 */
    for (int i = 0; i < 2; i++) {  /* Map 8KB to be safe */
        unsigned long page_pa = 0x10000000UL + ((unsigned long)i << PTE_SHIFT);
        int pte_idx = (0x10000000UL >> PTE_SHIFT) & 0x1FF;
        uart_pgtable[pte_idx + i] = (page_pa >> 12) << PTE_PPN_SHIFT;
        uart_pgtable[pte_idx + i] |= PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;
    }

    /* Map 0x20000000-0x3FFFFFFF using gigapage */
    pa = 0x20000000UL;
    root_pgtable[1] = (pa >> 12) << PTE_PPN_SHIFT;
    root_pgtable[1] |= PTE_V | PTE_R | PTE_W | PTE_A | PTE_D;

    /* Map 2GB-3GB (0x80000000-0xBFFFFFFF) as R/W/X for kernel */
    pa = 0x80000000UL;
    root_pgtable[2] = (pa >> 12) << PTE_PPN_SHIFT;
    root_pgtable[2] |= PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;

    early_puts("[MMU] Page table setup complete\n");

    /* Set SATP register - root_pgtable is in BSS, identity mapped */
    unsigned long root_pa = virt_to_phys((unsigned long)root_pgtable);
    satp_value = (root_pa >> 12) | (8UL << 60);  /* SV39 mode */

    return 0;
}

/* Enable virtual memory */
void enable_mmu(void)
{
    /* Set SATP register */
    asm volatile ("csrw satp, %0" :: "r"(satp_value));

    /* Flush TLB */
    asm volatile ("sfence.vma" : : : "memory");
}

/* Paging helper functions for SV39 page table manipulation */

/* Change page protection */
int protect_page(unsigned long va, unsigned long flags)
{
    pte_t *pte;

    pte = &root_pgtable[va >> PGDIR_SHIFT];

    if (*pte & PTE_V) {
        /* Clear all permission bits */
        *pte &= ~(PTE_R | PTE_W | PTE_X | PTE_U);
        /* Set new permissions */
        *pte |= flags;
        flush_tlb_page(va);
        return 0;
    }

    return -1;
}

/* Get physical address from virtual address */
unsigned long lookup_pa(unsigned long va)
{
    pte_t *pte;

    pte = &root_pgtable[va >> PGDIR_SHIFT];

    if (*pte & PTE_V) {
        return ((*pte >> PTE_PPN_SHIFT) << 12) | (va & (PAGE_SIZE - 1));
    }

    return 0;  /* Not mapped */
}

