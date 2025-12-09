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

/* Page directory */
static pte_t *root_pgtable = NULL;

/* Satp register value */
static unsigned long satp_value = 0;

/* Page allocation functions */
extern unsigned long alloc_page(void);
extern void free_page(unsigned long addr);
extern unsigned long phys_to_virt(unsigned long addr);
extern unsigned long virt_to_phys(unsigned long addr);

/* Initialize page table */
int pgtable_init(void)
{
    /* Allocate root page directory */
    unsigned long page = alloc_page();
    if (page == 0) {
        return -1;
    }

    root_pgtable = (pte_t *)phys_to_virt(page);

    /* Clear root page directory */
    for (int i = 0; i < PTRS_PER_PTE; i++) {
        root_pgtable[i] = 0;
    }

    /* Set up identity mapping for kernel space (0x80000000 - 0x81000000) */
    unsigned long va = 0x80000000UL;
    unsigned long end_va = 0x81000000UL;

    while (va < end_va) {
        /* PTE for 2MB page */
        pte_t *pte = &root_pgtable[va >> PGDIR_SHIFT];

        *pte = (va >> 12) << PTE_PPN_SHIFT;
        *pte |= PTE_V | PTE_R | PTE_W | PTE_X;

        va += (1UL << PGDIR_SHIFT);
    }

    /* Set SATP register */
    satp_value = ((unsigned long)virt_to_phys((unsigned long)root_pgtable) >> 12) |
                   (8UL << 60);  /* SV39 mode */

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

