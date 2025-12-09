/* Block Device Driver Implementation */

#include <minix/config.h>
#include <types.h>
#include <minix/blockdev.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of block devices */
#define MAX_BLOCK_DEVS  16

/* Block device registry */
static block_dev_t *block_devices[MAX_BLOCK_DEVS];
static int num_block_devices = 0;

/* Block cache */
#define BLOCK_CACHE_SIZE 64
typedef struct {
    void *dev_ptr;          /* Block device pointer */
    u32 block_num;
    void *data;
    int dirty;
} block_cache_entry_t;

static block_cache_entry_t block_cache[BLOCK_CACHE_SIZE];
static int cache_hits = 0;
static int cache_misses = 0;

/* Forward declarations */
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);

/**
 * Register a block device
 */
int blockdev_register(block_dev_t *dev)
{
    if (num_block_devices >= MAX_BLOCK_DEVS) {
        early_puts("BLOCKDEV: Too many devices registered\n");
        return -1;
    }

    if (dev == NULL || dev->ops == NULL) {
        early_puts("BLOCKDEV: Invalid device\n");
        return -1;
    }

    block_devices[num_block_devices] = dev;
    num_block_devices++;

    early_puts("BLOCKDEV: Registered device: ");
    early_puts(dev->name);
    early_puts(" (");
    early_puts("block_size=");
    /* TODO: print block_size */
    early_puts(")\n");

    return 0;
}

/**
 * Find a block device by name
 */
block_dev_t *blockdev_find(const char *name)
{
    int i;

    for (i = 0; i < num_block_devices; i++) {
        if (block_devices[i]) {
            const char *dev_name = block_devices[i]->name;
            const char *search_name = name;

            /* String comparison */
            while (*dev_name && *search_name && *dev_name == *search_name) {
                dev_name++;
                search_name++;
            }

            if (*dev_name == '\0' && *search_name == '\0') {
                return block_devices[i];
            }
        }
    }

    return NULL;
}

/**
 * Read a block from device
 * @dev: block device
 * @block_num: block number to read
 * @buf: buffer to read into
 * @count: number of blocks to read
 * @return: number of bytes read, or -1 on error
 */
ssize_t blockdev_read(block_dev_t *dev, u32 block_num, void *buf, u32 count)
{
    int i, found = -1;
    ssize_t result;

    if (dev == NULL || dev->ops == NULL || dev->ops->read_block == NULL) {
        return -1;
    }

    if (buf == NULL) {
        return -1;
    }

    /* Check block cache */
    for (i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].data &&
            block_cache[i].dev_ptr == (void *)dev &&
            block_cache[i].block_num == block_num) {
            /* Cache hit */
            cache_hits++;
            if (count == 1) {
                /* Copy from cache */
                u8 *src = (u8 *)block_cache[i].data;
                u8 *dst = (u8 *)buf;
                u32 size = dev->block_size;
                while (size--) {
                    *dst++ = *src++;
                }
                return dev->block_size;
            }
            break;
        }
    }

    cache_misses++;

    /* Not in cache or multiple blocks, read from device */
    result = dev->ops->read_block(block_num, buf, count);

    if (result > 0 && count == 1) {
        /* Add to cache */
        if (found == -1) {
            found = cache_misses % BLOCK_CACHE_SIZE;
        }
        block_cache[found].dev_ptr = (void *)dev;
        block_cache[found].block_num = block_num;
        block_cache[found].data = buf;
        block_cache[found].dirty = 0;
    }

    return result;
}

/**
 * Write a block to device
 * @dev: block device
 * @block_num: block number to write
 * @buf: buffer to write from
 * @count: number of blocks to write
 * @return: number of bytes written, or -1 on error
 */
ssize_t blockdev_write(block_dev_t *dev, u32 block_num, const void *buf, u32 count)
{
    int i;

    if (dev == NULL || dev->ops == NULL || dev->ops->write_block == NULL) {
        return -1;
    }

    if (buf == NULL) {
        return -1;
    }

    /* Mark cache as dirty */
    for (i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].data &&
            block_cache[i].dev_ptr == (void *)dev &&
            block_cache[i].block_num == block_num) {
            block_cache[i].dirty = 1;
            break;
        }
    }

    return dev->ops->write_block(block_num, buf, count);
}

/**
 * Flush cache
 */
int blockdev_flush(block_dev_t *dev)
{
    int i, result = 0;

    for (i = 0; i < BLOCK_CACHE_SIZE; i++) {
        if (block_cache[i].data &&
            block_cache[i].dev_ptr == (void *)dev &&
            block_cache[i].dirty) {
            /* Flush dirty block */
            if (dev->ops->write_block) {
                ssize_t res = dev->ops->write_block(
                    block_cache[i].block_num,
                    block_cache[i].data,
                    1);
                if (res < 0) {
                    result = -1;
                } else {
                    block_cache[i].dirty = 0;
                }
            }
        }
    }

    return result;
}

/**
 * Initialize block device subsystem
 */
int blockdev_init(void)
{
    int i;

    early_puts("✓ Block device ready\n");

    /* Initialize cache */
    for (i = 0; i < BLOCK_CACHE_SIZE; i++) {
        block_cache[i].data = NULL;
        block_cache[i].dev_ptr = NULL;
        block_cache[i].block_num = 0;
        block_cache[i].dirty = 0;
    }

    return 0;
}
