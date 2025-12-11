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
extern int devfs_init(void);
extern int ramfs_init(void);
extern int vfs_mount(const char *device, const char *mount_point, const char *fstype);

/* Initialize all device drivers */
void drivers_init(void)
{
    /* UART is already initialized in console_init() */

    /* Initialize block device subsystem */
    blockdev_init();

    /* Initialize filesystem support */
    vfs_init();

    /* Register filesystem types */
    devfs_init();
    ramfs_init();
    fat_init();
    fat32_init();
    ext2_init();
    ext3_init();
    ext4_init();

    /* Mount root filesystem (ramfs) */
    vfs_mount("none", "/", "ramfs");

    /* Mount devfs on /dev - temporarily disabled for debugging */
    /* vfs_mount("none", "/dev", "devfs"); */

    /* TODO: Initialize other drivers */
    /* GPIO driver */
    /* Timer driver */
    /* Interrupt controller driver */
}