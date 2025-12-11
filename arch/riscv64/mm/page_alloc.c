/* Buddy allocator for RISC-V 64-bit */

#include <minix/config.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Page size definitions */
#undef PAGE_SIZE
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           (~(PAGE_SIZE - 1))

/* Physical memory layout for QEMU virt machine */
#define PHYS_MEMORY_BASE    0x80000000UL
#define PHYS_MEMORY_SIZE    (128 * 1024 * 1024)  /* 128MB */

/* Kernel reserved memory (first 4MB for kernel code/data/stack) */
#define KERNEL_RESERVED     (4 * 1024 * 1024)

/* Buddy allocator parameters */
#define MAX_ORDER           11                   /* Max 2^11 = 2048 pages = 8MB */
#define BUDDY_MAX_PAGES     (1UL << MAX_ORDER)

/* Page structure for buddy allocator */
struct page {
    unsigned long flags;        /* Page flags */
    int order;                  /* Order if this is head of a free block */
    struct page *next;          /* Next in free list */
    struct page *prev;          /* Prev in free list */
    unsigned long ref_count;    /* Reference count */
};

/* Page flags */
#define PG_FREE         (1UL << 0)
#define PG_USED         (1UL << 1)
#define PG_RESERVED     (1UL << 2)
#define PG_HEAD         (1UL << 3)  /* Head of a compound page */

/* Free area structure for each order */
struct free_area {
    struct page *free_list;     /* Doubly linked list of free blocks */
    unsigned long nr_free;      /* Number of free blocks */
};

/* Global memory management data */
static struct page *mem_map = NULL;          /* Array of page structures */
static unsigned long mem_map_pages = 0;      /* Pages used for mem_map */
static unsigned long start_pfn = 0;          /* First managed page frame number */
static unsigned long end_pfn = 0;            /* Last managed page frame number */
static unsigned long total_pages = 0;        /* Total managed pages */
static unsigned long free_page_count = 0;    /* Current free pages */

/* Free areas for each order */
static struct free_area free_area[MAX_ORDER + 1];

/* External functions */
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);

/* Convert page frame number to physical address */
static inline unsigned long pfn_to_phys(unsigned long pfn)
{
    return pfn << PAGE_SHIFT;
}

/* Convert physical address to page frame number */
static inline unsigned long phys_to_pfn(unsigned long phys)
{
    return phys >> PAGE_SHIFT;
}

/* Get page structure from page frame number */
static inline struct page *pfn_to_page(unsigned long pfn)
{
    if (pfn < start_pfn || pfn >= end_pfn)
        return NULL;
    return &mem_map[pfn - start_pfn];
}

/* Get page frame number from page structure */
static inline unsigned long page_to_pfn(struct page *page)
{
    return (page - mem_map) + start_pfn;
}

/* Get physical address from page structure */
static inline unsigned long page_to_phys(struct page *page)
{
    return pfn_to_phys(page_to_pfn(page));
}

/* Add page to free list for given order */
static void list_add(struct free_area *area, struct page *page)
{
    page->prev = NULL;
    page->next = area->free_list;
    if (area->free_list) {
        area->free_list->prev = page;
    }
    area->free_list = page;
    area->nr_free++;
}

/* Remove page from free list */
static void list_del(struct free_area *area, struct page *page)
{
    if (page->prev) {
        page->prev->next = page->next;
    } else {
        area->free_list = page->next;
    }
    if (page->next) {
        page->next->prev = page->prev;
    }
    page->next = NULL;
    page->prev = NULL;
    area->nr_free--;
}

/* Find buddy page for a given page at specified order */
static struct page *find_buddy(struct page *page, int order)
{
    unsigned long pfn = page_to_pfn(page);
    unsigned long buddy_pfn = pfn ^ (1UL << order);

    if (buddy_pfn < start_pfn || buddy_pfn >= end_pfn)
        return NULL;

    return pfn_to_page(buddy_pfn);
}

/* Allocate pages of given order (2^order pages) */
unsigned long alloc_pages(int order)
{
    int current_order;
    struct page *page;
    struct page *buddy;

    if (order < 0 || order > MAX_ORDER)
        return 0;

    /* Find a free block of sufficient size */
    for (current_order = order; current_order <= MAX_ORDER; current_order++) {
        if (free_area[current_order].free_list) {
            /* Found a free block */
            page = free_area[current_order].free_list;
            list_del(&free_area[current_order], page);

            /* Split larger blocks down to requested size */
            while (current_order > order) {
                current_order--;
                buddy = page + (1 << current_order);

                /* Initialize buddy as free block */
                buddy->flags = PG_FREE | PG_HEAD;
                buddy->order = current_order;
                buddy->ref_count = 0;

                list_add(&free_area[current_order], buddy);
            }

            /* Mark page as used */
            page->flags = PG_USED | PG_HEAD;
            page->order = order;
            page->ref_count = 1;

            /* Update free page count */
            free_page_count -= (1 << order);

            return page_to_phys(page);
        }
    }

    return 0;  /* No memory available */
}

/* Free pages of given order */
void free_pages(unsigned long addr, int order)
{
    struct page *page;
    struct page *buddy;
    unsigned long pfn;

    if (addr == 0 || order < 0 || order > MAX_ORDER)
        return;

    pfn = phys_to_pfn(addr);
    page = pfn_to_page(pfn);

    if (!page || !(page->flags & PG_HEAD)) {
        early_puts("[BUDDY] ERROR: free_pages invalid page\n");
        return;
    }

    /* Decrease reference count */
    if (page->ref_count > 0) {
        page->ref_count--;
        if (page->ref_count > 0)
            return;  /* Still in use */
    }

    /* Mark as free */
    page->flags = PG_FREE | PG_HEAD;
    free_page_count += (1 << order);

    /* Try to coalesce with buddy */
    while (order < MAX_ORDER) {
        buddy = find_buddy(page, order);

        /* Check if buddy exists and is free at same order */
        if (!buddy || !(buddy->flags & PG_FREE) || buddy->order != order)
            break;

        /* Remove buddy from free list */
        list_del(&free_area[order], buddy);
        buddy->flags = 0;
        buddy->order = -1;

        /* Merge: use lower address as new page */
        if (buddy < page) {
            page = buddy;
        }

        order++;
        page->order = order;
    }

    /* Add to free list */
    list_add(&free_area[order], page);
}

/* Convenience function: allocate single page */
unsigned long alloc_page(void)
{
    return alloc_pages(0);
}

/* Convenience function: free single page */
void free_page(unsigned long addr)
{
    free_pages(addr, 0);
}

/* Get memory statistics */
void get_mem_info(unsigned long *total, unsigned long *free)
{
    if (total) *total = total_pages * PAGE_SIZE;
    if (free) *free = free_page_count * PAGE_SIZE;
}

/* Print buddy allocator statistics */
void buddy_stats(void)
{
    int order;

    early_puts("\n=== Buddy Allocator Statistics ===\n");
    early_puts("Total pages: ");
    early_puthex(total_pages);
    early_puts("\nFree pages:  ");
    early_puthex(free_page_count);
    early_puts("\nUsed pages:  ");
    early_puthex(total_pages - free_page_count);
    early_puts("\n\nFree blocks by order:\n");

    for (order = 0; order <= MAX_ORDER; order++) {
        if (free_area[order].nr_free > 0) {
            early_puts("  Order ");
            early_puthex(order);
            early_puts(" (");
            early_puthex((1UL << order) * PAGE_SIZE);
            early_puts(" bytes): ");
            early_puthex(free_area[order].nr_free);
            early_puts(" blocks\n");
        }
    }
}

/* Initialize page allocator with buddy system */
void page_init(void)
{
    unsigned long managed_start;
    unsigned long managed_end;
    unsigned long mem_map_size;
    unsigned long pfn;
    int order;

    early_puts("[BUDDY] Initializing buddy allocator...\n");

    /* Calculate managed memory region */
    managed_start = PHYS_MEMORY_BASE + KERNEL_RESERVED;
    managed_end = PHYS_MEMORY_BASE + PHYS_MEMORY_SIZE;

    start_pfn = phys_to_pfn(managed_start);
    end_pfn = phys_to_pfn(managed_end);
    total_pages = end_pfn - start_pfn;

    early_puts("[BUDDY] Memory range: ");
    early_puthex(managed_start);
    early_puts(" - ");
    early_puthex(managed_end);
    early_puts("\n");

    /* Calculate space needed for mem_map array */
    mem_map_size = total_pages * sizeof(struct page);
    mem_map_pages = (mem_map_size + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Place mem_map at the beginning of managed memory */
    mem_map = (struct page *)managed_start;

    early_puts("[BUDDY] Page array: ");
    early_puthex((unsigned long)mem_map);
    early_puts(" (");
    early_puthex(mem_map_pages);
    early_puts(" pages)\n");

    /* Initialize all page structures */
    for (pfn = start_pfn; pfn < end_pfn; pfn++) {
        struct page *page = pfn_to_page(pfn);
        page->flags = 0;
        page->order = -1;
        page->next = NULL;
        page->prev = NULL;
        page->ref_count = 0;
    }

    /* Initialize free areas */
    for (order = 0; order <= MAX_ORDER; order++) {
        free_area[order].free_list = NULL;
        free_area[order].nr_free = 0;
    }

    /* Mark pages used by mem_map as reserved */
    for (pfn = start_pfn; pfn < start_pfn + mem_map_pages; pfn++) {
        struct page *page = pfn_to_page(pfn);
        page->flags = PG_RESERVED;
    }

    /* Add remaining pages to buddy system */
    /* Start from first page after mem_map, aligned to max order */
    pfn = start_pfn + mem_map_pages;

    /* Ensure starting pfn is at least 2-page aligned for THREAD_SIZE allocations */
    /* This guarantees alloc_pages(1) returns 8KB-aligned addresses */
    if (pfn & 1) {
        struct page *page = pfn_to_page(pfn);
        page->flags = PG_RESERVED;  /* Mark unaligned page as reserved */
        pfn++;
    }

    free_page_count = 0;

    /* Add pages in largest possible chunks */
    while (pfn < end_pfn) {
        /* Find largest order that fits */
        for (order = MAX_ORDER; order >= 0; order--) {
            unsigned long block_size = 1UL << order;
            unsigned long aligned_pfn = (pfn + block_size - 1) & ~(block_size - 1);

            /* Check if we can allocate this block */
            if (aligned_pfn == pfn && pfn + block_size <= end_pfn) {
                struct page *page = pfn_to_page(pfn);
                page->flags = PG_FREE | PG_HEAD;
                page->order = order;
                list_add(&free_area[order], page);
                free_page_count += block_size;
                pfn += block_size;
                break;
            }
        }

        /* If no order fits (shouldn't happen), skip one page */
        if (order < 0) {
            pfn++;
        }
    }

    early_puts("[BUDDY] Total managed pages: ");
    early_puthex(total_pages);
    early_puts("\n[BUDDY] Free pages: ");
    early_puthex(free_page_count);
    early_puts("\n[BUDDY] Reserved pages: ");
    early_puthex(total_pages - free_page_count);
    early_puts("\n[BUDDY] Buddy allocator initialized\n");
}

/* Map page to virtual address - stub for now */
unsigned long map_page(unsigned long phys_addr, unsigned long virt_addr, int flags)
{
    /* Will be implemented in pgtable.c */
    (void)phys_addr;
    (void)virt_addr;
    (void)flags;
    return 0;
}

/* Unmap page - stub for now */
void unmap_page(unsigned long virt_addr)
{
    (void)virt_addr;
}

/* Convert virtual to physical address (identity mapping for now) */
unsigned long virt_to_phys(unsigned long virt_addr)
{
    return virt_addr;
}

/* Convert physical to virtual address (identity mapping for now) */
unsigned long phys_to_virt(unsigned long phys_addr)
{
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
    asm volatile ("sfence.vma" ::: "memory");
}
