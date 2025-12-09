/* Device drivers initialization */

#include <minix/config.h>

/* Forward declarations for block device driver */
extern int blockdev_init(void);

/* Forward declarations for filesystem drivers */
extern int vfs_init(void);
extern int fat_init(void);
extern int fat32_init(void);
extern int ext2_init(void);
extern int ext3_init(void);
extern int ext4_init(void);

/* Initialize all device drivers */
void drivers_init(void)
{
    /* UART is already initialized in console_init() */

    /* Initialize block device subsystem */
    blockdev_init();

    /* Initialize filesystem support */
    vfs_init();
    fat_init();
    fat32_init();
    ext2_init();
    ext3_init();
    ext4_init();

    /* TODO: Initialize other drivers */
    /* GPIO driver */
    /* Timer driver */
    /* Interrupt controller driver */
}