/* Memory Management Header */

#ifndef _MINIX_MM_H
#define _MINIX_MM_H

#include <types.h>

/* Page size definitions */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           (~(PAGE_SIZE - 1))

/* Page table entry flags */
#define PTE_V               (1UL << 0)    /* Valid */
#define PTE_R               (1UL << 1)    /* Read */
#define PTE_W               (1UL << 2)    /* Write */
#define PTE_X               (1UL << 3)    /* Execute */
#define PTE_U               (1UL << 4)    /* User accessible */
#define PTE_G               (1UL << 5)    /* Global */
#define PTE_A               (1UL << 6)    /* Accessed */
#define PTE_D               (1UL << 7)    /* Dirty */

/* Common flag combinations */
#define PTE_KERNEL_RW       (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)
#define PTE_KERNEL_RO       (PTE_V | PTE_R | PTE_A)
#define PTE_KERNEL_RX       (PTE_V | PTE_R | PTE_X | PTE_A)
#define PTE_KERNEL_RWX      (PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D)
#define PTE_USER_RW         (PTE_V | PTE_R | PTE_W | PTE_U | PTE_A | PTE_D)
#define PTE_USER_RO         (PTE_V | PTE_R | PTE_U | PTE_A)
#define PTE_USER_RX         (PTE_V | PTE_R | PTE_X | PTE_U | PTE_A)
#define PTE_USER_RWX        (PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D)

/* ============================================
 * Memory Management Initialization
 * ============================================ */

/* Initialize all memory management subsystems */
void mm_init(void);

/* ============================================
 * Physical Page Allocator (Buddy System)
 * ============================================ */

/* Allocate 2^order contiguous pages, returns physical address */
unsigned long alloc_pages(int order);

/* Free 2^order contiguous pages */
void free_pages(unsigned long addr, int order);

/* Allocate single page (convenience function) */
unsigned long alloc_page(void);

/* Free single page (convenience function) */
void free_page(unsigned long addr);

/* Get memory statistics */
void get_mem_info(unsigned long *total, unsigned long *free);

/* Print buddy allocator statistics */
void buddy_stats(void);

/* ============================================
 * Slab Allocator (Object Allocator)
 * ============================================ */

/* Slab cache structure (opaque) */
struct slab_cache;

/* Create a new slab cache for objects of given size */
struct slab_cache *kmem_cache_create(const char *name, unsigned long size);

/* Destroy a slab cache */
void kmem_cache_destroy(struct slab_cache *cache);

/* Allocate object from a slab cache */
void *kmem_cache_alloc(struct slab_cache *cache);

/* Free object back to slab cache */
void kmem_cache_free(struct slab_cache *cache, void *obj);

/* Generic kernel memory allocation (like malloc) */
void *kmalloc(unsigned long size);

/* Free kmalloc'd memory */
void kfree(void *ptr);

/* Debug functions */
void kmalloc_stats(void);
void kmalloc_dump(void);
int kmalloc_verify(void);

/* ============================================
 * Page Table Management
 * ============================================ */

/* Page table entry types */
typedef unsigned long pte_t;
typedef unsigned long pgd_t;

/* Initialize kernel page tables */
int pgtable_init(void);

/* Enable MMU */
void enable_mmu(void);

/* Get kernel page directory */
pgd_t *get_kernel_pgd(void);

/* Map a 4KB page */
int map_page_4k(pgd_t *pgd, unsigned long va, unsigned long pa, unsigned long flags);

/* Map a 2MB megapage */
int map_page_2m(pgd_t *pgd, unsigned long va, unsigned long pa, unsigned long flags);

/* Map a 1GB gigapage */
int map_page_1g(pgd_t *pgd, unsigned long va, unsigned long pa, unsigned long flags);

/* Unmap a page */
void unmap_page_pte(pgd_t *pgd, unsigned long va);

/* Map a region with 4KB pages */
int map_region(pgd_t *pgd, unsigned long va_start, unsigned long pa_start,
               unsigned long size, unsigned long flags);

/* Map a region using largest possible pages */
int map_region_large(pgd_t *pgd, unsigned long va_start, unsigned long pa_start,
                     unsigned long size, unsigned long flags);

/* Look up physical address for virtual address */
unsigned long lookup_pa(unsigned long va);

/* Change page protection flags */
int protect_page(unsigned long va, unsigned long flags);

/* TLB operations */
void flush_tlb_all(void);
void flush_tlb_page(unsigned long addr);
void flush_tlb_mm(unsigned long asid);
void flush_tlb_range(unsigned long start, unsigned long size);

/* Address conversion (identity mapping for now) */
unsigned long virt_to_phys(unsigned long virt_addr);
unsigned long phys_to_virt(unsigned long phys_addr);

/* Debug */
void dump_pte(unsigned long va);

/* ============================================
 * Virtual Memory Allocator (vmalloc)
 * ============================================ */

/* Initialize vmalloc subsystem */
void vmalloc_init(void);

/* Allocate virtually contiguous kernel memory */
void *vmalloc(unsigned long size);

/* Allocate zeroed virtually contiguous kernel memory */
void *vzalloc(unsigned long size);

/* Free vmalloc'd memory */
void vfree(void *addr);

/* Map array of pages to contiguous virtual region */
void *vmap(unsigned long *pages, unsigned long nr_pages, unsigned long flags);

/* Unmap vmap'd region (does not free physical pages) */
void vunmap(void *addr);

/* Map physical I/O memory */
void *ioremap(unsigned long phys_addr, unsigned long size);

/* Unmap ioremap'd region */
void iounmap(void *addr);

/* Debug */
void vmalloc_dump(void);

#endif /* _MINIX_MM_H */
