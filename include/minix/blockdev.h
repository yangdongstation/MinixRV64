/* Block Device Interface */

#ifndef _MINIX_BLOCKDEV_H
#define _MINIX_BLOCKDEV_H

#include <types.h>

/* Block device operations */
typedef struct block_dev_ops {
    /* Read block from device */
    int (*read_block)(u32 block_num, void *buf, u32 count);

    /* Write block to device */
    int (*write_block)(u32 block_num, const void *buf, u32 count);

    /* Get block size */
    u32 (*get_block_size)(void);

    /* Get total blocks */
    u64 (*get_total_blocks)(void);

    /* Device name */
    const char *name;
} block_dev_ops_t;

/* Block device instance */
typedef struct block_dev {
    const char *name;
    block_dev_ops_t *ops;
    void *private;
    u32 block_size;
    u64 total_blocks;
} block_dev_t;

#endif /* _MINIX_BLOCKDEV_H */
