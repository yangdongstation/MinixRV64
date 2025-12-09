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

/* Slab object states */
#define SLAB_UNUSED     0
#define SLAB_PARTIAL   1
#define SLAB_FULL       2
#define SLAB_FREE       3

/* Maximum slab size */
#define SLAB_MAX_SIZE   2048

/* Slab cache structure */
struct slab_cache {
    char name[32];
    unsigned long object_size;
    unsigned long objects_per_slab;
    unsigned long free_objects;
    struct slab *slabs;
    struct slab *partial;
    void **free_objects_list;
};

/* Slab structure */
struct slab {
    unsigned long *objects;        /* Array of objects */
    unsigned long flags;           /* State flags */
    struct slab *next;             /* Next slab in cache */
    struct slab_cache *cache;       /* Parent cache */
    unsigned long inuse;           /* Objects in use */
};

/* Common slab caches */
static struct slab_cache kmem_cache_cache;
static struct slab_cache *slab_caches[16];
static int num_slab_caches = 0;

/* Allocate a slab */
static struct slab *alloc_slab(struct slab_cache *cache)
{
    struct slab *slab;
    unsigned long page;
    unsigned long obj_addr;
    int i;

    /* Allocate a page */
    page = alloc_page();
    if (page == 0) {
        return NULL;
    }

    /* Allocate slab structure */
    slab = (struct slab *)page;
    obj_addr = page + sizeof(struct slab);

    /* Align objects */
    obj_addr = (obj_addr + cache->object_size - 1) & ~(cache->object_size - 1);

    /* Initialize object array */
    slab->objects = (unsigned long *)obj_addr;
    slab->flags = SLAB_PARTIAL;
    slab->next = NULL;
    slab->cache = cache;
    slab->inuse = 0;

    /* Initialize free list */
    cache->free_objects_list = (void **)obj_addr;
    for (i = 0; i < (int)cache->objects_per_slab - 1; i++) {
        cache->free_objects_list[i] = (void *)(obj_addr + i * cache->object_size);
    }
    cache->free_objects_list[i] = NULL;
    cache->free_objects = cache->objects_per_slab;

    return slab;
}

/* Create a new slab cache */
struct slab_cache *kmem_cache_create(const char *name, unsigned long size)
{
    struct slab_cache *cache;

    (void)name;  /* Suppress unused parameter warning */

    if (num_slab_caches >= 16) {
        return NULL;
    }

    cache = &kmem_cache_cache;
    cache->object_size = size;
    cache->objects_per_slab = (PAGE_SIZE - sizeof(struct slab)) / size;
    cache->free_objects = 0;
    cache->slabs = NULL;
    cache->partial = NULL;

    /* Store cache pointer */
    slab_caches[num_slab_caches++] = cache;

    return cache;
}

/* Allocate object from cache */
void *kmem_cache_alloc(struct slab_cache *cache)
{
    struct slab *slab;
    void *obj = NULL;

    /* Try to find a slab with free objects */
    slab = cache->partial;
    if (!slab) {
        /* Allocate new slab */
        slab = alloc_slab(cache);
        if (!slab) {
            return NULL;
        }
        cache->partial = slab;
        if (!cache->slabs) {
            cache->slabs = slab;
        }
    }

    /* Allocate object */
    if (cache->free_objects > 0) {
        obj = cache->free_objects_list[0];
        cache->free_objects_list = (void **)cache->free_objects_list[0];
        cache->free_objects--;
        slab->inuse++;

        /* Update slab state */
        if (cache->free_objects == 0) {
            slab->flags = SLAB_FULL;
            cache->partial = NULL;
        }
    }

    return obj;
}

/* Free object to cache */
void kmem_cache_free(struct slab_cache *cache, void *obj)
{
    struct slab *slab;
    unsigned long page;

    /* Find which slab this object belongs to */
    page = ((unsigned long)obj & PAGE_MASK);
    slab = (struct slab *)page;

    if (slab && slab->cache == cache) {
        /* Add to free list */
        cache->free_objects_list[cache->free_objects] = obj;
        cache->free_objects++;
        slab->inuse--;

        /* Update slab state */
        if (slab->flags == SLAB_FULL) {
            slab->flags = SLAB_PARTIAL;
            slab->next = cache->partial;
            cache->partial = slab;
        }
    }
}

/* Simple kmalloc implementation */
void *kmalloc(unsigned long size)
{
    struct slab_cache *cache;
    int cache_index;

    /* Find appropriate cache */
    if (size <= 64) {
        cache_index = 0;
        size = 64;
    } else if (size <= 128) {
        cache_index = 1;
        size = 128;
    } else if (size <= 256) {
        cache_index = 2;
        size = 256;
    } else if (size <= 512) {
        cache_index = 3;
        size = 512;
    } else if (size <= 1024) {
        cache_index = 4;
        size = 1024;
    } else {
        /* Allocate full page */
        return (void *)alloc_page();
    }

    /* Get or create cache */
    if (cache_index < num_slab_caches) {
        cache = slab_caches[cache_index];
    } else {
        /* Use fixed size names instead of sprintf */
        cache = kmem_cache_create("generic", size);
        if (!cache) {
            return NULL;
        }
    }

    return kmem_cache_alloc(cache);
}

/* Free memory */
void kfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    /* For now, just free the page */
    /* TODO: Implement proper kfree with cache tracking */
    free_page((unsigned long)ptr & PAGE_MASK);
}

/* Initialize slab allocator */
void kmem_init(void)
{
    /* Create basic caches */
    kmem_cache_create("kmem_cache", sizeof(struct slab_cache));
    kmem_cache_create("size-64", 64);
    kmem_cache_create("size-128", 128);
    kmem_cache_create("size-256", 256);
    kmem_cache_create("size-512", 512);
    kmem_cache_create("size-1024", 1024);
}