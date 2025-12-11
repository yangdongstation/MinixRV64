/* RISC-V SV39 page table management */

#include <minix/config.h>
#include <types.h>
#include <asm/csr.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Page size definitions */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           (~(PAGE_SIZE - 1))

/* SV39 virtual address layout:
 * Bits 38:30 - VPN[2] (PGD index)
 * Bits 29:21 - VPN[1] (PMD index)
 * Bits 20:12 - VPN[0] (PTE index)
 * Bits 11:0  - Page offset
 */
#define PGDIR_SHIFT         30
#define PMD_SHIFT           21
#define PTE_SHIFT           12

/* Entries per page table level */
#define PTRS_PER_PGD        512
#define PTRS_PER_PMD        512
#define PTRS_PER_PTE        512

/* Virtual address masks */
#define PGD_MASK            ((PTRS_PER_PGD - 1) << PGDIR_SHIFT)
#define PMD_MASK            ((PTRS_PER_PMD - 1) << PMD_SHIFT)
#define PTE_MASK            ((PTRS_PER_PTE - 1) << PTE_SHIFT)

/* Page table entry flags */
#define PTE_V               (1UL << 0)    /* Valid */
#define PTE_R               (1UL << 1)    /* Read */
#define PTE_W               (1UL << 2)    /* Write */
#define PTE_X               (1UL << 3)    /* Execute */
#define PTE_U               (1UL << 4)    /* User accessible */
#define PTE_G               (1UL << 5)    /* Global */
#define PTE_A               (1UL << 6)    /* Accessed */
#define PTE_D               (1UL << 7)    /* Dirty */
#define PTE_PPN_SHIFT       10

/* Convenience flag combinations */
#define PTE_KERNEL          (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)
#define PTE_KERNEL_RO       (PTE_V | PTE_R | PTE_A)
#define PTE_KERNEL_EXEC     (PTE_V | PTE_R | PTE_X | PTE_A)
#define PTE_KERNEL_RWX      (PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D)
#define PTE_USER_RO         (PTE_V | PTE_R | PTE_U | PTE_A)
#define PTE_USER_RW         (PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D)
#define PTE_USER_RX         (PTE_V | PTE_R | PTE_X | PTE_U | PTE_A)
#define PTE_USER_RWX        (PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D)

/* SATP register - use values from csr.h, calculate properly for SV39 */
#define SATP_SV39_MODE      (8UL << 60)
#define SATP_BARE_MODE      (0UL << 60)
#define SATP_ASID_SHIFT     44
#define SATP_PPN_MASK       0xFFFFFFFFFFFUL

/* Page table entry type */
typedef unsigned long pte_t;
typedef unsigned long pgd_t;
typedef unsigned long pmd_t;

/* Kernel virtual address space layout (for future use):
 * 0x0000_0000_0000_0000 - 0x0000_003F_FFFF_FFFF : User space (256GB)
 * 0xFFFF_FFC0_0000_0000 - 0xFFFF_FFDF_FFFF_FFFF : Direct mapping (physical)
 * 0xFFFF_FFE0_0000_0000 - 0xFFFF_FFEF_FFFF_FFFF : vmalloc area
 * 0xFFFF_FFF0_0000_0000 - 0xFFFF_FFFF_FFFF_FFFF : Fixed mappings
 *
 * For now, we use identity mapping (VA == PA) until we need kernel virtual addresses.
 */

/* Memory layout constants */
#define PHYS_MEMORY_BASE    0x80000000UL
#define MMIO_BASE           0x00000000UL
#define MMIO_END            0x40000000UL  /* 1GB for MMIO */

/* Root page table - 4KB aligned */
static pgd_t kernel_pgd[PTRS_PER_PGD] __attribute__((aligned(4096)));

/* SATP value for kernel */
static unsigned long kernel_satp = 0;

/* External functions */
extern unsigned long alloc_page(void);
extern void free_page(unsigned long addr);
extern unsigned long virt_to_phys(unsigned long addr);
extern unsigned long phys_to_virt(unsigned long addr);
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);

/* Get PGD index from virtual address */
static inline unsigned long pgd_index(unsigned long va)
{
    return (va >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1);
}

/* Get PMD index from virtual address */
static inline unsigned long pmd_index(unsigned long va)
{
    return (va >> PMD_SHIFT) & (PTRS_PER_PMD - 1);
}

/* Get PTE index from virtual address */
static inline unsigned long pte_index(unsigned long va)
{
    return (va >> PTE_SHIFT) & (PTRS_PER_PTE - 1);
}

/* Check if entry is valid */
static inline int pte_valid(pte_t pte)
{
    return pte & PTE_V;
}

/* Check if entry is a leaf (has R/W/X permissions) */
static inline int pte_leaf(pte_t pte)
{
    return (pte & (PTE_R | PTE_W | PTE_X)) != 0;
}

/* Get physical address from page table entry */
static inline unsigned long pte_to_phys(pte_t pte)
{
    return (pte >> PTE_PPN_SHIFT) << PAGE_SHIFT;
}

/* Create page table entry from physical address */
static inline pte_t phys_to_pte(unsigned long phys, unsigned long flags)
{
    return ((phys >> PAGE_SHIFT) << PTE_PPN_SHIFT) | flags;
}

/* Allocate a page table (returns physical address) */
static unsigned long pgtable_alloc(void)
{
    unsigned long page = alloc_page();
    if (page) {
        /* Zero out the page table */
        unsigned long *p = (unsigned long *)page;
        for (int i = 0; i < 512; i++) {
            p[i] = 0;
        }
    }
    return page;
}

/* Free a page table (for future use) */
static void pgtable_free(unsigned long page) __attribute__((unused));
static void pgtable_free(unsigned long page)
{
    free_page(page);
}

/* Walk page table and optionally create intermediate tables */
static pte_t *walk_pgtable(pgd_t *pgd, unsigned long va, int create)
{
    pmd_t *pmd_table;
    pte_t *pte_table;
    unsigned long idx;
    unsigned long phys;

    /* Level 2: PGD */
    idx = pgd_index(va);
    if (!pte_valid(pgd[idx])) {
        if (!create)
            return NULL;

        phys = pgtable_alloc();
        if (!phys)
            return NULL;

        pgd[idx] = phys_to_pte(phys, PTE_V);
    }

    /* Check for gigapage (leaf at PGD level) */
    if (pte_leaf(pgd[idx])) {
        return &pgd[idx];
    }

    /* Level 1: PMD */
    pmd_table = (pmd_t *)pte_to_phys(pgd[idx]);
    idx = pmd_index(va);
    if (!pte_valid(pmd_table[idx])) {
        if (!create)
            return NULL;

        phys = pgtable_alloc();
        if (!phys)
            return NULL;

        pmd_table[idx] = phys_to_pte(phys, PTE_V);
    }

    /* Check for megapage (leaf at PMD level) */
    if (pte_leaf(pmd_table[idx])) {
        return &pmd_table[idx];
    }

    /* Level 0: PTE */
    pte_table = (pte_t *)pte_to_phys(pmd_table[idx]);
    idx = pte_index(va);

    return &pte_table[idx];
}

/* Map a single 4KB page */
int map_page_4k(pgd_t *pgd, unsigned long va, unsigned long pa, unsigned long flags)
{
    pte_t *pte;

    /* Align addresses */
    va &= PAGE_MASK;
    pa &= PAGE_MASK;

    pte = walk_pgtable(pgd, va, 1);
    if (!pte)
        return -1;

    /* Set the mapping */
    *pte = phys_to_pte(pa, flags | PTE_V);

    /* Flush TLB for this address */
    asm volatile ("sfence.vma %0" :: "r"(va) : "memory");

    return 0;
}

/* Map a 2MB megapage */
int map_page_2m(pgd_t *pgd, unsigned long va, unsigned long pa, unsigned long flags)
{
    pmd_t *pmd_table;
    unsigned long pgd_idx, pmd_idx;
    unsigned long phys;

    /* Align to 2MB boundary */
    va &= ~((1UL << PMD_SHIFT) - 1);
    pa &= ~((1UL << PMD_SHIFT) - 1);

    /* Get/create PGD entry */
    pgd_idx = pgd_index(va);
    if (!pte_valid(pgd[pgd_idx])) {
        phys = pgtable_alloc();
        if (!phys)
            return -1;
        pgd[pgd_idx] = phys_to_pte(phys, PTE_V);
    }

    /* Get PMD table */
    pmd_table = (pmd_t *)pte_to_phys(pgd[pgd_idx]);
    pmd_idx = pmd_index(va);

    /* Set megapage mapping (leaf entry at PMD level) */
    pmd_table[pmd_idx] = phys_to_pte(pa, flags | PTE_V);

    asm volatile ("sfence.vma %0" :: "r"(va) : "memory");

    return 0;
}

/* Map a 1GB gigapage */
int map_page_1g(pgd_t *pgd, unsigned long va, unsigned long pa, unsigned long flags)
{
    unsigned long pgd_idx;

    /* Align to 1GB boundary */
    va &= ~((1UL << PGDIR_SHIFT) - 1);
    pa &= ~((1UL << PGDIR_SHIFT) - 1);

    pgd_idx = pgd_index(va);

    /* Set gigapage mapping (leaf entry at PGD level) */
    pgd[pgd_idx] = phys_to_pte(pa, flags | PTE_V);

    asm volatile ("sfence.vma %0" :: "r"(va) : "memory");

    return 0;
}

/* Unmap a single page */
void unmap_page_pte(pgd_t *pgd, unsigned long va)
{
    pte_t *pte;

    va &= PAGE_MASK;
    pte = walk_pgtable(pgd, va, 0);
    if (pte && pte_valid(*pte)) {
        *pte = 0;
        asm volatile ("sfence.vma %0" :: "r"(va) : "memory");
    }
}

/* Map a memory region with 4KB pages */
int map_region(pgd_t *pgd, unsigned long va_start, unsigned long pa_start,
               unsigned long size, unsigned long flags)
{
    unsigned long va = va_start & PAGE_MASK;
    unsigned long pa = pa_start & PAGE_MASK;
    unsigned long end = (va_start + size + PAGE_SIZE - 1) & PAGE_MASK;

    while (va < end) {
        if (map_page_4k(pgd, va, pa, flags) < 0)
            return -1;
        va += PAGE_SIZE;
        pa += PAGE_SIZE;
    }

    return 0;
}

/* Map a memory region using largest possible pages */
int map_region_large(pgd_t *pgd, unsigned long va_start, unsigned long pa_start,
                     unsigned long size, unsigned long flags)
{
    unsigned long va = va_start;
    unsigned long pa = pa_start;
    unsigned long end = va_start + size;

    while (va < end) {
        unsigned long remaining = end - va;

        /* Try 1GB gigapage */
        if ((va & ((1UL << PGDIR_SHIFT) - 1)) == 0 &&
            (pa & ((1UL << PGDIR_SHIFT) - 1)) == 0 &&
            remaining >= (1UL << PGDIR_SHIFT)) {
            if (map_page_1g(pgd, va, pa, flags) < 0)
                return -1;
            va += 1UL << PGDIR_SHIFT;
            pa += 1UL << PGDIR_SHIFT;
        }
        /* Try 2MB megapage */
        else if ((va & ((1UL << PMD_SHIFT) - 1)) == 0 &&
                 (pa & ((1UL << PMD_SHIFT) - 1)) == 0 &&
                 remaining >= (1UL << PMD_SHIFT)) {
            if (map_page_2m(pgd, va, pa, flags) < 0)
                return -1;
            va += 1UL << PMD_SHIFT;
            pa += 1UL << PMD_SHIFT;
        }
        /* Use 4KB page */
        else {
            if (map_page_4k(pgd, va, pa, flags) < 0)
                return -1;
            va += PAGE_SIZE;
            pa += PAGE_SIZE;
        }
    }

    return 0;
}

/* Look up physical address for a virtual address */
unsigned long lookup_pa(unsigned long va)
{
    pte_t *pte = walk_pgtable(kernel_pgd, va, 0);

    if (!pte || !pte_valid(*pte))
        return 0;

    return pte_to_phys(*pte) | (va & (PAGE_SIZE - 1));
}

/* Change page protection flags */
int protect_page(unsigned long va, unsigned long flags)
{
    pte_t *pte = walk_pgtable(kernel_pgd, va, 0);

    if (!pte || !pte_valid(*pte))
        return -1;

    /* Preserve PPN, update flags */
    unsigned long phys = pte_to_phys(*pte);
    *pte = phys_to_pte(phys, flags | PTE_V);

    asm volatile ("sfence.vma %0" :: "r"(va) : "memory");

    return 0;
}

/* TLB flush operations */
void flush_tlb_mm(unsigned long asid)
{
    if (asid == 0) {
        asm volatile ("sfence.vma" ::: "memory");
    } else {
        asm volatile ("sfence.vma zero, %0" :: "r"(asid) : "memory");
    }
}

void flush_tlb_range(unsigned long start, unsigned long size)
{
    unsigned long end = start + size;
    (void)end;  /* For now just flush all */
    asm volatile ("sfence.vma" ::: "memory");
}

/* Initialize kernel page tables */
int pgtable_init(void)
{
    unsigned long i;

    early_puts("[MMU] Initializing SV39 page tables...\n");

    /* Clear the root page table */
    for (i = 0; i < PTRS_PER_PGD; i++) {
        kernel_pgd[i] = 0;
    }

    /* Map MMIO region: 0x00000000-0x3FFFFFFF (1GB) as identity mapped */
    early_puts("[MMU] Mapping MMIO: 0x00000000 - 0x3FFFFFFF\n");
    if (map_page_1g(kernel_pgd, MMIO_BASE, MMIO_BASE, PTE_KERNEL) < 0) {
        early_puts("[MMU] ERROR: Failed to map MMIO region\n");
        return -1;
    }

    /* Map kernel memory: 0x80000000-0xBFFFFFFF (1GB) as identity mapped */
    early_puts("[MMU] Mapping Kernel: 0x80000000 - 0xBFFFFFFF\n");
    if (map_page_1g(kernel_pgd, PHYS_MEMORY_BASE, PHYS_MEMORY_BASE, PTE_KERNEL_RWX) < 0) {
        early_puts("[MMU] ERROR: Failed to map kernel region\n");
        return -1;
    }

    /* Compute SATP value */
    kernel_satp = SATP_SV39_MODE | (virt_to_phys((unsigned long)kernel_pgd) >> PAGE_SHIFT);

    early_puts("[MMU] Root PGD: ");
    early_puthex((unsigned long)kernel_pgd);
    early_puts("\n[MMU] SATP value: ");
    early_puthex(kernel_satp);
    early_puts("\n[MMU] Page table initialization complete\n");

    return 0;
}

/* Enable MMU with kernel page table */
void enable_mmu(void)
{
    unsigned long readback;

    early_puts("[MMU] Enabling MMU...\n");

    /* Set SATP register */
    asm volatile ("csrw satp, %0" :: "r"(kernel_satp));

    /* Flush TLB */
    asm volatile ("sfence.vma" ::: "memory");

    /* Read back to verify */
    asm volatile ("csrr %0, satp" : "=r"(readback));

    if (readback != kernel_satp) {
        early_puts("[MMU] WARNING: SATP readback mismatch!\n");
        early_puts("[MMU]   Expected: ");
        early_puthex(kernel_satp);
        early_puts("\n[MMU]   Got: ");
        early_puthex(readback);
        early_puts("\n");
    } else {
        early_puts("[MMU] SATP verified OK\n");
    }

    early_puts("[MMU] MMU enabled successfully\n");
}

/* Get kernel page directory */
pgd_t *get_kernel_pgd(void)
{
    return kernel_pgd;
}

/* Debug: dump page table entry */
void dump_pte(unsigned long va)
{
    pte_t *pte = walk_pgtable(kernel_pgd, va, 0);

    early_puts("VA ");
    early_puthex(va);
    early_puts(": ");

    if (!pte) {
        early_puts("not mapped (no entry)\n");
        return;
    }

    if (!pte_valid(*pte)) {
        early_puts("not mapped (invalid)\n");
        return;
    }

    early_puts("PTE ");
    early_puthex(*pte);
    early_puts(" -> PA ");
    early_puthex(pte_to_phys(*pte));
    early_puts(" flags: ");

    if (*pte & PTE_R) early_puts("R");
    if (*pte & PTE_W) early_puts("W");
    if (*pte & PTE_X) early_puts("X");
    if (*pte & PTE_U) early_puts("U");
    if (*pte & PTE_G) early_puts("G");
    if (*pte & PTE_A) early_puts("A");
    if (*pte & PTE_D) early_puts("D");
    early_puts("\n");
}
