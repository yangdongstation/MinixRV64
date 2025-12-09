/* Page allocator for RISC-V 64-bit */

#include <minix/config.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Page size is 4KB - use the one from config.h */
#undef PAGE_SIZE
#define PAGE_SIZE          4096
#define PAGE_MASK          (~(PAGE_SIZE - 1))

/* Physical memory layout */
#define PHYS_MEMORY_BASE     0x80000000UL
#define PHYS_MEMORY_SIZE     (128 * 1024 * 1024)  /* 128MB for QEMU */

/* Memory management structures */
struct page {
    unsigned long flags;    /* Page flags */
    unsigned long ref_count; /* Reference count */
    struct page *next;      /* Next free page */
};

/* Page flags */
#define PAGE_FREE     0x0
#define PAGE_USED     0x1
#define PAGE_RESERVED 0x2

/* Global memory information */
static struct page *pages = NULL;
static unsigned long total_pages = 0;
static unsigned long free_pages = 0;
static struct page *free_list = NULL;

/* Initialize page allocator */
void page_init(void)
{
    unsigned long mem_size;
    unsigned long page_count;
    unsigned long i;
    unsigned long start_page;
    unsigned long end_page;

    /* Calculate total memory size */
    mem_size = PHYS_MEMORY_SIZE;
    page_count = mem_size / PAGE_SIZE;

    /* Reserve pages for kernel (first 2MB) */
    start_page = (2 * 1024 * 1024) / PAGE_SIZE;
    end_page = page_count;

    total_pages = end_page - start_page;
    free_pages = total_pages;

    /* Allocate page array */
    pages = (struct page *)(PHYS_MEMORY_BASE + (2 * 1024 * 1024));

    /* Initialize free list */
    free_list = NULL;
    for (i = start_page; i < end_page; i++) {
        unsigned long page_idx = i - start_page;
        pages[page_idx].flags = PAGE_FREE;
        pages[page_idx].ref_count = 0;
        pages[page_idx].next = free_list;
        free_list = &pages[page_idx];
    }
}

/* Allocate a physical page */
unsigned long alloc_page(void)
{
    unsigned long page_addr = 0;

    if (free_list) {
        struct page *page = free_list;
        free_list = page->next;

        page->flags = PAGE_USED;
        page->ref_count = 1;
        page->next = NULL;

        free_pages--;

        /* Calculate physical address */
        page_addr = PHYS_MEMORY_BASE + (2 * 1024 * 1024) +
                   ((page - pages) * PAGE_SIZE);
    }

    return page_addr;
}

/* Free a physical page */
void free_page(unsigned long page_addr)
{
    unsigned long offset;
    unsigned long page_idx;

    /* Check if address is in managed memory range */
    offset = page_addr - (PHYS_MEMORY_BASE + (2 * 1024 * 1024));
    if (offset >= PHYS_MEMORY_SIZE - (2 * 1024 * 1024)) {
        return;  /* Not a managed page */
    }

    page_idx = offset / PAGE_SIZE;
    if (page_idx >= total_pages) {
        return;  /* Invalid page */
    }

    /* Free the page */
    if (pages[page_idx].ref_count > 0) {
        pages[page_idx].ref_count--;

        if (pages[page_idx].ref_count == 0) {
            pages[page_idx].flags = PAGE_FREE;
            pages[page_idx].next = free_list;
            free_list = &pages[page_idx];
            free_pages++;
        }
    }
}

/* Get memory statistics */
void get_mem_info(unsigned long *total, unsigned long *free)
{
    if (total) *total = total_pages * PAGE_SIZE;
    if (free) *free = free_pages * PAGE_SIZE;
}

/* Map page to virtual address */
unsigned long map_page(unsigned long phys_addr, unsigned long virt_addr, int flags)
{
    /* TODO: Implement page table mapping */
    /* This would set up SV39 page table entries */
    (void)phys_addr;
    (void)virt_addr;
    (void)flags;

    return 0;  /* Success */
}

/* Unmap page */
void unmap_page(unsigned long virt_addr)
{
    /* TODO: Implement page table unmapping */
    (void)virt_addr;
}

/* Convert virtual to physical address */
unsigned long virt_to_phys(unsigned long virt_addr)
{
    /* TODO: Implement virtual to physical translation */
    /* For now, assume direct mapping */
    return virt_addr;
}

/* Convert physical to virtual address */
unsigned long phys_to_virt(unsigned long phys_addr)
{
    /* TODO: Implement physical to virtual translation */
    /* For now, assume direct mapping */
    return phys_addr;
}

/* Flush TLB entry */
void flush_tlb_page(unsigned long addr)
{
    asm volatile ("sfence.vma %0" :: "r"(addr) : "memory");
}

/* Flush all TLB entries */
void flush_tlb_all(void)
{
    asm volatile ("sfence.vma" : : : "memory");
}