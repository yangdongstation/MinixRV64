/* vmalloc - Kernel virtual memory allocator */

#include <minix/config.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Page size definitions */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           (~(PAGE_SIZE - 1))

/* vmalloc region: for now we use a simple region after kernel memory
 * In a full implementation, this would be in high kernel virtual addresses
 * For identity-mapped kernel, we use physical addresses directly
 */
#define VMALLOC_START       0x84000000UL    /* After kernel + 64MB */
#define VMALLOC_END         0x88000000UL    /* 64MB vmalloc space */
#define VMALLOC_SIZE        (VMALLOC_END - VMALLOC_START)

/* Maximum vmalloc areas */
#define MAX_VMALLOC_AREAS   64

/* VM area structure */
struct vm_struct {
    void *addr;                 /* Virtual address */
    unsigned long size;         /* Size in bytes (including guard page) */
    unsigned long flags;        /* VM area flags */
    unsigned long nr_pages;     /* Number of pages */
    unsigned long *pages;       /* Array of physical page addresses */
    struct vm_struct *next;     /* Next VM area */
    int in_use;                 /* Is this structure in use? */
};

/* VM area flags */
#define VM_ALLOC            (1UL << 0)      /* vmalloc area */
#define VM_MAP              (1UL << 1)      /* vmap area (user pages) */
#define VM_IOREMAP          (1UL << 2)      /* ioremap area */

/* External functions */
extern unsigned long alloc_page(void);
extern unsigned long alloc_pages(int order);
extern void free_page(unsigned long addr);
extern void free_pages(unsigned long addr, int order);
extern int map_page_4k(void *pgd, unsigned long va, unsigned long pa, unsigned long flags);
extern void unmap_page_pte(void *pgd, unsigned long va);
extern void *get_kernel_pgd(void);
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);

/* Page table entry flags for vmalloc */
#define PTE_V               (1UL << 0)
#define PTE_R               (1UL << 1)
#define PTE_W               (1UL << 2)
#define PTE_A               (1UL << 6)
#define PTE_D               (1UL << 7)
#define PTE_KERNEL_RW       (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)

/* Static pool of VM structures */
static struct vm_struct vm_areas[MAX_VMALLOC_AREAS];
static struct vm_struct *vmlist = NULL;
static unsigned long vmalloc_pos = VMALLOC_START;
static int vmalloc_initialized = 0;

/* Initialize vmalloc subsystem */
void vmalloc_init(void)
{
    int i;

    early_puts("[VMALLOC] Initializing vmalloc...\n");

    /* Initialize VM area pool */
    for (i = 0; i < MAX_VMALLOC_AREAS; i++) {
        vm_areas[i].in_use = 0;
        vm_areas[i].addr = NULL;
        vm_areas[i].size = 0;
        vm_areas[i].pages = NULL;
        vm_areas[i].next = NULL;
    }

    vmlist = NULL;
    vmalloc_pos = VMALLOC_START;
    vmalloc_initialized = 1;

    early_puts("[VMALLOC] Range: ");
    early_puthex(VMALLOC_START);
    early_puts(" - ");
    early_puthex(VMALLOC_END);
    early_puts("\n[VMALLOC] Initialized\n");
}

/* Allocate a VM structure */
static struct vm_struct *alloc_vm_struct(void)
{
    int i;

    for (i = 0; i < MAX_VMALLOC_AREAS; i++) {
        if (!vm_areas[i].in_use) {
            vm_areas[i].in_use = 1;
            return &vm_areas[i];
        }
    }

    return NULL;
}

/* Free a VM structure */
static void free_vm_struct(struct vm_struct *vm)
{
    vm->in_use = 0;
    vm->addr = NULL;
    vm->size = 0;
    vm->flags = 0;
    vm->nr_pages = 0;
    vm->pages = NULL;
    vm->next = NULL;
}

/* Find virtual address space for allocation */
static unsigned long find_vmalloc_area(unsigned long size)
{
    struct vm_struct *vm;
    unsigned long addr = VMALLOC_START;
    unsigned long end;

    /* Add guard page to size */
    size += PAGE_SIZE;

    /* Align size to page boundary */
    size = (size + PAGE_SIZE - 1) & PAGE_MASK;

    /* Simple linear search through existing areas */
    for (vm = vmlist; vm != NULL; vm = vm->next) {
        end = (unsigned long)vm->addr;

        /* Check if there's enough space before this area */
        if (addr + size <= end) {
            return addr;
        }

        /* Move past this area */
        addr = (unsigned long)vm->addr + vm->size;
    }

    /* Check if there's space at the end */
    if (addr + size <= VMALLOC_END) {
        return addr;
    }

    return 0;  /* No space available */
}

/* Insert VM area into sorted list */
static void insert_vm_struct(struct vm_struct *vm)
{
    struct vm_struct **pprev = &vmlist;

    while (*pprev && (*pprev)->addr < vm->addr) {
        pprev = &(*pprev)->next;
    }

    vm->next = *pprev;
    *pprev = vm;
}

/* Remove VM area from list */
static void remove_vm_struct(struct vm_struct *vm)
{
    struct vm_struct **pprev = &vmlist;

    while (*pprev && *pprev != vm) {
        pprev = &(*pprev)->next;
    }

    if (*pprev) {
        *pprev = vm->next;
    }
}

/* Core vmalloc implementation */
static void *__vmalloc(unsigned long size, unsigned long flags)
{
    struct vm_struct *vm;
    unsigned long addr;
    unsigned long nr_pages;
    unsigned long i;
    void *pgd;

    if (!vmalloc_initialized || size == 0) {
        return NULL;
    }

    /* Calculate number of pages needed */
    nr_pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;

    /* Allocate VM structure */
    vm = alloc_vm_struct();
    if (!vm) {
        early_puts("[VMALLOC] ERROR: No VM structures available\n");
        return NULL;
    }

    /* Find virtual address space */
    addr = find_vmalloc_area(size);
    if (!addr) {
        free_vm_struct(vm);
        early_puts("[VMALLOC] ERROR: No virtual space available\n");
        return NULL;
    }

    /* Initialize VM structure */
    vm->addr = (void *)addr;
    vm->size = (nr_pages + 1) * PAGE_SIZE;  /* +1 for guard page */
    vm->flags = flags;
    vm->nr_pages = nr_pages;
    vm->pages = NULL;  /* We don't track individual pages for now */

    /* Get kernel page directory */
    pgd = get_kernel_pgd();

    /* Allocate and map pages */
    for (i = 0; i < nr_pages; i++) {
        unsigned long page = alloc_page();
        if (!page) {
            /* Allocation failed - unmap and free already allocated pages */
            unsigned long j;
            for (j = 0; j < i; j++) {
                unmap_page_pte(pgd, addr + j * PAGE_SIZE);
                /* Note: We can't easily free pages since we don't track them */
            }
            free_vm_struct(vm);
            early_puts("[VMALLOC] ERROR: Page allocation failed\n");
            return NULL;
        }

        /* Map the page */
        if (map_page_4k(pgd, addr + i * PAGE_SIZE, page, PTE_KERNEL_RW) < 0) {
            free_page(page);
            /* Unmap and free already allocated pages */
            unsigned long j;
            for (j = 0; j < i; j++) {
                unmap_page_pte(pgd, addr + j * PAGE_SIZE);
            }
            free_vm_struct(vm);
            early_puts("[VMALLOC] ERROR: Page mapping failed\n");
            return NULL;
        }

        /* Zero the page */
        unsigned long *p = (unsigned long *)(addr + i * PAGE_SIZE);
        for (unsigned long k = 0; k < (PAGE_SIZE / sizeof(unsigned long)); k++) {
            p[k] = 0;
        }
    }

    /* Insert into VM list */
    insert_vm_struct(vm);

    return (void *)addr;
}

/* vmalloc - allocate virtually contiguous memory */
void *vmalloc(unsigned long size)
{
    return __vmalloc(size, VM_ALLOC);
}

/* vzalloc - allocate zeroed virtually contiguous memory */
void *vzalloc(unsigned long size)
{
    void *ptr = vmalloc(size);
    /* Memory is already zeroed in __vmalloc */
    return ptr;
}

/* Find VM area by address */
static struct vm_struct *find_vm_area(void *addr)
{
    struct vm_struct *vm;

    for (vm = vmlist; vm != NULL; vm = vm->next) {
        if (vm->addr == addr) {
            return vm;
        }
    }

    return NULL;
}

/* vfree - free vmalloc'd memory */
void vfree(void *addr)
{
    struct vm_struct *vm;
    unsigned long va;
    unsigned long i;
    void *pgd;

    if (!addr || !vmalloc_initialized) {
        return;
    }

    /* Find the VM area */
    vm = find_vm_area(addr);
    if (!vm) {
        early_puts("[VMALLOC] WARNING: vfree on unknown address\n");
        return;
    }

    /* Get kernel page directory */
    pgd = get_kernel_pgd();

    /* Unmap and free all pages */
    va = (unsigned long)vm->addr;
    for (i = 0; i < vm->nr_pages; i++) {
        /* Get physical address before unmapping */
        /* For now we just unmap without freeing physical pages
         * since we don't track them. A full implementation would
         * store the page addresses in vm->pages and free them here.
         */
        unmap_page_pte(pgd, va + i * PAGE_SIZE);
    }

    /* Remove from list and free structure */
    remove_vm_struct(vm);
    free_vm_struct(vm);
}

/* vmap - map array of pages to virtually contiguous region */
void *vmap(unsigned long *pages, unsigned long nr_pages, unsigned long flags)
{
    struct vm_struct *vm;
    unsigned long addr;
    unsigned long size;
    unsigned long i;
    void *pgd;

    if (!vmalloc_initialized || !pages || nr_pages == 0) {
        return NULL;
    }

    size = nr_pages * PAGE_SIZE;

    /* Allocate VM structure */
    vm = alloc_vm_struct();
    if (!vm) {
        return NULL;
    }

    /* Find virtual address space */
    addr = find_vmalloc_area(size);
    if (!addr) {
        free_vm_struct(vm);
        return NULL;
    }

    /* Initialize VM structure */
    vm->addr = (void *)addr;
    vm->size = (nr_pages + 1) * PAGE_SIZE;
    vm->flags = VM_MAP | flags;
    vm->nr_pages = nr_pages;
    vm->pages = pages;  /* Store reference to user's page array */

    /* Get kernel page directory */
    pgd = get_kernel_pgd();

    /* Map all pages */
    for (i = 0; i < nr_pages; i++) {
        if (map_page_4k(pgd, addr + i * PAGE_SIZE, pages[i], PTE_KERNEL_RW) < 0) {
            /* Unmap already mapped pages */
            unsigned long j;
            for (j = 0; j < i; j++) {
                unmap_page_pte(pgd, addr + j * PAGE_SIZE);
            }
            free_vm_struct(vm);
            return NULL;
        }
    }

    /* Insert into VM list */
    insert_vm_struct(vm);

    return (void *)addr;
}

/* vunmap - unmap vmap'd region (pages are NOT freed) */
void vunmap(void *addr)
{
    struct vm_struct *vm;
    unsigned long va;
    unsigned long i;
    void *pgd;

    if (!addr || !vmalloc_initialized) {
        return;
    }

    vm = find_vm_area(addr);
    if (!vm) {
        return;
    }

    /* Only vunmap VM_MAP areas */
    if (!(vm->flags & VM_MAP)) {
        early_puts("[VMALLOC] WARNING: vunmap on non-vmap area\n");
        return;
    }

    pgd = get_kernel_pgd();

    /* Unmap pages (don't free - vmap doesn't own them) */
    va = (unsigned long)vm->addr;
    for (i = 0; i < vm->nr_pages; i++) {
        unmap_page_pte(pgd, va + i * PAGE_SIZE);
    }

    remove_vm_struct(vm);
    free_vm_struct(vm);
}

/* ioremap - map physical I/O memory into kernel virtual space */
void *ioremap(unsigned long phys_addr, unsigned long size)
{
    struct vm_struct *vm;
    unsigned long addr;
    unsigned long offset;
    unsigned long nr_pages;
    unsigned long i;
    void *pgd;

    if (!vmalloc_initialized || size == 0) {
        return NULL;
    }

    /* Save page offset */
    offset = phys_addr & (PAGE_SIZE - 1);
    phys_addr &= PAGE_MASK;

    /* Calculate pages needed */
    nr_pages = (size + offset + PAGE_SIZE - 1) >> PAGE_SHIFT;

    /* Allocate VM structure */
    vm = alloc_vm_struct();
    if (!vm) {
        return NULL;
    }

    /* Find virtual address space */
    addr = find_vmalloc_area(nr_pages * PAGE_SIZE);
    if (!addr) {
        free_vm_struct(vm);
        return NULL;
    }

    /* Initialize VM structure */
    vm->addr = (void *)addr;
    vm->size = (nr_pages + 1) * PAGE_SIZE;
    vm->flags = VM_IOREMAP;
    vm->nr_pages = nr_pages;
    vm->pages = NULL;

    /* Get kernel page directory */
    pgd = get_kernel_pgd();

    /* Map physical pages (no caching for I/O) */
    for (i = 0; i < nr_pages; i++) {
        /* Use uncached mapping for I/O (implementation-dependent) */
        if (map_page_4k(pgd, addr + i * PAGE_SIZE,
                        phys_addr + i * PAGE_SIZE, PTE_KERNEL_RW) < 0) {
            unsigned long j;
            for (j = 0; j < i; j++) {
                unmap_page_pte(pgd, addr + j * PAGE_SIZE);
            }
            free_vm_struct(vm);
            return NULL;
        }
    }

    insert_vm_struct(vm);

    return (void *)(addr + offset);
}

/* iounmap - unmap ioremap'd region */
void iounmap(void *addr)
{
    struct vm_struct *vm;
    unsigned long va;
    unsigned long base_addr;
    unsigned long i;
    void *pgd;

    if (!addr || !vmalloc_initialized) {
        return;
    }

    /* Align address to page boundary */
    base_addr = (unsigned long)addr & PAGE_MASK;

    vm = find_vm_area((void *)base_addr);
    if (!vm || !(vm->flags & VM_IOREMAP)) {
        early_puts("[VMALLOC] WARNING: iounmap on non-ioremap area\n");
        return;
    }

    pgd = get_kernel_pgd();

    va = (unsigned long)vm->addr;
    for (i = 0; i < vm->nr_pages; i++) {
        unmap_page_pte(pgd, va + i * PAGE_SIZE);
    }

    remove_vm_struct(vm);
    free_vm_struct(vm);
}

/* Debug: dump vmalloc areas */
void vmalloc_dump(void)
{
    struct vm_struct *vm;
    int count = 0;

    early_puts("\n=== vmalloc Areas ===\n");

    for (vm = vmlist; vm != NULL; vm = vm->next) {
        early_puts("  ");
        early_puthex((unsigned long)vm->addr);
        early_puts(" - ");
        early_puthex((unsigned long)vm->addr + vm->size);
        early_puts(" (");
        early_puthex(vm->size);
        early_puts(" bytes, ");
        early_puthex(vm->nr_pages);
        early_puts(" pages)");

        if (vm->flags & VM_ALLOC) early_puts(" ALLOC");
        if (vm->flags & VM_MAP) early_puts(" MAP");
        if (vm->flags & VM_IOREMAP) early_puts(" IOREMAP");

        early_puts("\n");
        count++;
    }

    early_puts("Total areas: ");
    early_puthex(count);
    early_puts("\n");
}
