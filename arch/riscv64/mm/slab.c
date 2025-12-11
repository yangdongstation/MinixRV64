/* Slab allocator for kernel objects */

#include <minix/config.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define PAGE_SIZE          4096
#define PAGE_MASK          (~(PAGE_SIZE - 1))

/* External functions */
extern unsigned long alloc_page(void);
extern void free_page(unsigned long addr);
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);

/* Slab states */
#define SLAB_PARTIAL    0
#define SLAB_FULL       1
#define SLAB_EMPTY      2

/* Maximum slab caches */
#define MAX_SLAB_CACHES 16

/* Minimum object size (must hold a pointer for free list) */
#define MIN_OBJ_SIZE    sizeof(void *)

/* Slab structure - placed at the beginning of each slab page */
struct slab {
    struct slab *next;              /* Next slab in list */
    struct slab *prev;              /* Previous slab in list */
    struct slab_cache *cache;       /* Parent cache */
    void *free_list;                /* Free object list */
    unsigned int inuse;             /* Number of objects in use */
    unsigned int total;             /* Total objects in this slab */
};

/* Slab cache structure */
struct slab_cache {
    const char *name;               /* Cache name for debugging */
    unsigned long obj_size;         /* Size of each object */
    unsigned long align;            /* Alignment requirement */
    unsigned int objs_per_slab;     /* Objects per slab */
    unsigned int slab_count;        /* Number of slabs */
    struct slab *slabs_partial;     /* Partially used slabs */
    struct slab *slabs_full;        /* Fully used slabs */
    struct slab *slabs_empty;       /* Empty slabs (for caching) */
    unsigned long total_allocs;     /* Statistics: total allocations */
    unsigned long total_frees;      /* Statistics: total frees */
};

/* Global slab caches for kmalloc */
static struct slab_cache *kmalloc_caches[8];  /* 32, 64, 128, 256, 512, 1024, 2048, 4096 */
static struct slab_cache all_caches[MAX_SLAB_CACHES];
static int num_caches = 0;
static int slab_initialized = 0;

/* Simple string copy (for future use) */
static void strcpy_safe(char *dst, const char *src, int max) __attribute__((unused));
static void strcpy_safe(char *dst, const char *src, int max)
{
    int i;
    for (i = 0; i < max - 1 && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* Initialize a slab with free objects */
static void slab_init_objs(struct slab *slab, struct slab_cache *cache)
{
    unsigned long base;
    unsigned long obj_addr;
    void **prev_ptr;
    unsigned int i;

    /* Objects start after the slab header, aligned */
    base = (unsigned long)slab + sizeof(struct slab);
    base = (base + cache->align - 1) & ~(cache->align - 1);

    /* Build free list - each free object contains pointer to next */
    prev_ptr = &slab->free_list;
    for (i = 0; i < cache->objs_per_slab; i++) {
        obj_addr = base + i * cache->obj_size;
        *prev_ptr = (void *)obj_addr;
        prev_ptr = (void **)obj_addr;
    }
    *prev_ptr = NULL;  /* End of list */

    slab->inuse = 0;
    slab->total = cache->objs_per_slab;
}

/* Allocate a new slab */
static struct slab *slab_alloc(struct slab_cache *cache)
{
    struct slab *slab;
    unsigned long page;

    /* Allocate a page for the slab */
    page = alloc_page();
    if (page == 0) {
        return NULL;
    }

    /* Initialize slab header */
    slab = (struct slab *)page;
    slab->next = NULL;
    slab->prev = NULL;
    slab->cache = cache;

    /* Initialize objects */
    slab_init_objs(slab, cache);

    cache->slab_count++;

    return slab;
}

/* Free a slab */
static void slab_free(struct slab *slab)
{
    slab->cache->slab_count--;
    free_page((unsigned long)slab);
}

/* Add slab to a list */
static void slab_list_add(struct slab **list, struct slab *slab)
{
    slab->prev = NULL;
    slab->next = *list;
    if (*list) {
        (*list)->prev = slab;
    }
    *list = slab;
}

/* Remove slab from a list */
static void slab_list_remove(struct slab **list, struct slab *slab)
{
    if (slab->prev) {
        slab->prev->next = slab->next;
    } else {
        *list = slab->next;
    }
    if (slab->next) {
        slab->next->prev = slab->prev;
    }
    slab->next = NULL;
    slab->prev = NULL;
}

/* Create a new slab cache */
struct slab_cache *kmem_cache_create(const char *name, unsigned long size)
{
    struct slab_cache *cache;
    unsigned long obj_size;
    unsigned long usable_space;

    if (num_caches >= MAX_SLAB_CACHES) {
        return NULL;
    }

    /* Get a cache structure */
    cache = &all_caches[num_caches++];

    /* Calculate object size with alignment */
    obj_size = size;
    if (obj_size < MIN_OBJ_SIZE) {
        obj_size = MIN_OBJ_SIZE;
    }
    /* Align to 8 bytes */
    obj_size = (obj_size + 7) & ~7UL;

    /* Initialize cache */
    cache->name = name;
    cache->obj_size = obj_size;
    cache->align = 8;

    /* Calculate objects per slab */
    usable_space = PAGE_SIZE - sizeof(struct slab);
    usable_space = usable_space & ~(cache->align - 1);  /* Align */
    cache->objs_per_slab = usable_space / obj_size;

    if (cache->objs_per_slab == 0) {
        num_caches--;
        return NULL;  /* Object too large for slab */
    }

    cache->slab_count = 0;
    cache->slabs_partial = NULL;
    cache->slabs_full = NULL;
    cache->slabs_empty = NULL;
    cache->total_allocs = 0;
    cache->total_frees = 0;

    return cache;
}

/* Destroy a slab cache */
void kmem_cache_destroy(struct slab_cache *cache)
{
    struct slab *slab, *next;

    /* Free all slabs */
    for (slab = cache->slabs_partial; slab; slab = next) {
        next = slab->next;
        slab_free(slab);
    }
    for (slab = cache->slabs_full; slab; slab = next) {
        next = slab->next;
        slab_free(slab);
    }
    for (slab = cache->slabs_empty; slab; slab = next) {
        next = slab->next;
        slab_free(slab);
    }

    cache->slabs_partial = NULL;
    cache->slabs_full = NULL;
    cache->slabs_empty = NULL;
}

/* Allocate object from cache */
void *kmem_cache_alloc(struct slab_cache *cache)
{
    struct slab *slab;
    void *obj;

    if (!cache) {
        return NULL;
    }

    /* Try to find a slab with free objects */
    slab = cache->slabs_partial;

    if (!slab) {
        /* Try empty slabs first */
        if (cache->slabs_empty) {
            slab = cache->slabs_empty;
            slab_list_remove(&cache->slabs_empty, slab);
            slab_list_add(&cache->slabs_partial, slab);
        } else {
            /* Allocate new slab */
            slab = slab_alloc(cache);
            if (!slab) {
                return NULL;
            }
            slab_list_add(&cache->slabs_partial, slab);
        }
    }

    /* Get object from free list */
    obj = slab->free_list;
    slab->free_list = *(void **)obj;
    slab->inuse++;

    /* Move slab to full list if needed */
    if (slab->inuse == slab->total) {
        slab_list_remove(&cache->slabs_partial, slab);
        slab_list_add(&cache->slabs_full, slab);
    }

    cache->total_allocs++;

    return obj;
}

/* Free object to cache */
void kmem_cache_free(struct slab_cache *cache, void *obj)
{
    struct slab *slab;
    unsigned long page;
    int was_full;

    if (!cache || !obj) {
        return;
    }

    /* Find which slab this object belongs to */
    page = (unsigned long)obj & PAGE_MASK;
    slab = (struct slab *)page;

    /* Verify it belongs to this cache */
    if (slab->cache != cache) {
        early_puts("[SLAB] ERROR: kmem_cache_free wrong cache!\n");
        return;
    }

    was_full = (slab->inuse == slab->total);

    /* Add object back to free list */
    *(void **)obj = slab->free_list;
    slab->free_list = obj;
    slab->inuse--;

    /* Move slab between lists as needed */
    if (was_full) {
        slab_list_remove(&cache->slabs_full, slab);
        slab_list_add(&cache->slabs_partial, slab);
    } else if (slab->inuse == 0) {
        slab_list_remove(&cache->slabs_partial, slab);
        /* Keep one empty slab for caching, free the rest */
        if (cache->slabs_empty == NULL) {
            slab_list_add(&cache->slabs_empty, slab);
        } else {
            slab_free(slab);
        }
    }

    cache->total_frees++;
}

/* Get cache index for size */
static int kmalloc_index(unsigned long size)
{
    if (size <= 32) return 0;
    if (size <= 64) return 1;
    if (size <= 128) return 2;
    if (size <= 256) return 3;
    if (size <= 512) return 4;
    if (size <= 1024) return 5;
    if (size <= 2048) return 6;
    if (size <= 4096) return 7;
    return -1;  /* Too large */
}

/* Simple kmalloc implementation */
void *kmalloc(unsigned long size)
{
    int index;
    struct slab_cache *cache;

    if (size == 0) {
        return NULL;
    }

    if (!slab_initialized) {
        /* Fallback: allocate full page before slab is initialized */
        if (size <= PAGE_SIZE) {
            return (void *)alloc_page();
        }
        return NULL;
    }

    index = kmalloc_index(size);
    if (index < 0) {
        /* Too large for slab, allocate pages directly */
        unsigned long pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        if (pages_needed == 1) {
            return (void *)alloc_page();
        }
        /* TODO: Use buddy allocator for multi-page allocations */
        return NULL;
    }

    cache = kmalloc_caches[index];
    if (!cache) {
        return NULL;
    }

    return kmem_cache_alloc(cache);
}

/* Free memory - determines cache from object address */
void kfree(void *ptr)
{
    struct slab *slab;
    unsigned long page;

    if (!ptr) {
        return;
    }

    if (!slab_initialized) {
        /* Before slab init, just free the page */
        free_page((unsigned long)ptr & PAGE_MASK);
        return;
    }

    /* Find the slab header */
    page = (unsigned long)ptr & PAGE_MASK;
    slab = (struct slab *)page;

    /* Check if this is a slab-allocated object */
    if (slab->cache && slab->cache >= &all_caches[0] &&
        slab->cache < &all_caches[MAX_SLAB_CACHES]) {
        kmem_cache_free(slab->cache, ptr);
    } else {
        /* Not a slab allocation, free the page directly */
        free_page(page);
    }
}

/* Debug: print slab cache statistics */
void kmalloc_stats(void)
{
    int i;
    struct slab_cache *cache;

    early_puts("\n=== Slab Allocator Statistics ===\n");

    for (i = 0; i < 8; i++) {
        cache = kmalloc_caches[i];
        if (cache) {
            early_puts("Cache: ");
            early_puts(cache->name);
            early_puts("  size=");
            early_puthex(cache->obj_size);
            early_puts("  slabs=");
            early_puthex(cache->slab_count);
            early_puts("  allocs=");
            early_puthex(cache->total_allocs);
            early_puts("  frees=");
            early_puthex(cache->total_frees);
            early_puts("\n");
        }
    }
}

/* Debug: dump slab cache info */
void kmalloc_dump(void)
{
    int i;
    struct slab_cache *cache;
    struct slab *slab;

    early_puts("\n=== Slab Allocator Dump ===\n");

    for (i = 0; i < 8; i++) {
        cache = kmalloc_caches[i];
        if (!cache) continue;

        early_puts("\nCache: ");
        early_puts(cache->name);
        early_puts("\n");

        early_puts("  Partial slabs:\n");
        for (slab = cache->slabs_partial; slab; slab = slab->next) {
            early_puts("    slab @ ");
            early_puthex((unsigned long)slab);
            early_puts("  inuse=");
            early_puthex(slab->inuse);
            early_puts("/");
            early_puthex(slab->total);
            early_puts("\n");
        }

        early_puts("  Full slabs: ");
        int count = 0;
        for (slab = cache->slabs_full; slab; slab = slab->next) count++;
        early_puthex(count);
        early_puts("\n");

        early_puts("  Empty slabs: ");
        count = 0;
        for (slab = cache->slabs_empty; slab; slab = slab->next) count++;
        early_puthex(count);
        early_puts("\n");
    }
}

/* Debug: verify slab allocator integrity */
int kmalloc_verify(void)
{
    int i;
    struct slab_cache *cache;
    struct slab *slab;
    int errors = 0;

    for (i = 0; i < 8; i++) {
        cache = kmalloc_caches[i];
        if (!cache) continue;

        /* Check partial slabs */
        for (slab = cache->slabs_partial; slab; slab = slab->next) {
            if (slab->cache != cache) {
                early_puts("[SLAB] ERROR: slab->cache mismatch\n");
                errors++;
            }
            if (slab->inuse >= slab->total) {
                early_puts("[SLAB] ERROR: partial slab is full\n");
                errors++;
            }
            if (slab->inuse == 0) {
                early_puts("[SLAB] ERROR: partial slab is empty\n");
                errors++;
            }
        }

        /* Check full slabs */
        for (slab = cache->slabs_full; slab; slab = slab->next) {
            if (slab->inuse != slab->total) {
                early_puts("[SLAB] ERROR: full slab not full\n");
                errors++;
            }
        }

        /* Check empty slabs */
        for (slab = cache->slabs_empty; slab; slab = slab->next) {
            if (slab->inuse != 0) {
                early_puts("[SLAB] ERROR: empty slab not empty\n");
                errors++;
            }
        }
    }

    return errors;
}

/* Initialize slab allocator */
void kmem_init(void)
{
    static const unsigned long sizes[] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };
    static const char *names[] = {
        "size-32", "size-64", "size-128", "size-256",
        "size-512", "size-1024", "size-2048", "size-4096"
    };
    int i;

    early_puts("[SLAB] Initializing slab allocator...\n");

    /* Create kmalloc caches */
    for (i = 0; i < 8; i++) {
        kmalloc_caches[i] = kmem_cache_create(names[i], sizes[i]);
        if (!kmalloc_caches[i]) {
            early_puts("[SLAB] ERROR: Failed to create cache ");
            early_puts(names[i]);
            early_puts("\n");
        }
    }

    slab_initialized = 1;

    early_puts("[SLAB] Slab allocator initialized\n");
}
