/* Block Device Private Interface */

#ifndef _MINIX_BLOCKDEV_PRIV_H
#define _MINIX_BLOCKDEV_PRIV_H

#include <types.h>
#include <minix/blockdev.h>

/* Block device functions */
int blockdev_init(void);
int blockdev_register(block_dev_t *dev);
block_dev_t *blockdev_find(const char *name);
ssize_t blockdev_read(block_dev_t *dev, u32 block_num, void *buf, u32 count);
ssize_t blockdev_write(block_dev_t *dev, u32 block_num, const void *buf, u32 count);
int blockdev_flush(block_dev_t *dev);

#endif /* _MINIX_BLOCKDEV_PRIV_H */
