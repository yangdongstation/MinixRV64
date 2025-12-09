/* FAT32 Filesystem Implementation */

#include <minix/config.h>
#include <types.h>
#include <minix/vfs.h>
#include <minix/blockdev.h>
#include <minix/blockdev_priv.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* FAT32 Constants */
#define FAT32_BOOT_SIGNATURE    0xAA55
#define FAT32_FSINFO_SIGNATURE  0x41615252
#define FAT32_FSINFO_TRAIL_SIG  0x61417272
#define FAT32_EOC_MARKER        0x0FFFFFFF  /* End of chain marker */

/* FSINFO Sector */
typedef struct {
    u32 lead_sig;               /* 0x41615252 */
    u8 reserved1[480];
    u32 struc_sig;              /* 0x61417272 */
    u32 free_count;             /* Free cluster count */
    u32 next_free;              /* Next free cluster */
    u8 reserved2[12];
    u32 trail_sig;              /* 0xAA550000 */
} fat32_fsinfo_t;

/* FAT32 Boot Sector */
typedef struct {
    u8 jmp[3];
    u8 oem[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 num_fats;
    u16 root_entries;
    u16 total_sectors_16;
    u8 media;
    u16 fat_size_16;
    u16 sectors_per_track;
    u16 num_heads;
    u32 hidden_sectors;
    u32 total_sectors_32;
    u32 fat_size_32;
    u16 ext_flags;
    u16 fs_version;
    u32 root_cluster;
    u16 fsinfo_sector;
    u16 backup_boot_sector;
    u8 reserved[12];
    u8 drive_number;
    u8 reserved2;
    u8 boot_signature;
    u32 volume_id;
    u8 volume_label[11];
    u8 fs_type[8];
} fat32_boot_t;

/* FAT32 Directory Entry */
typedef struct {
    u8 name[11];
    u8 attr;
    u8 reserved;
    u8 create_time_tenth;
    u16 create_time;
    u16 create_date;
    u16 access_date;
    u16 first_cluster_high;
    u16 write_time;
    u16 write_date;
    u16 first_cluster_low;
    u32 file_size;
} fat32_dirent_t;

/* File attributes */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME     0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN        0x0F

/* FAT32 filesystem structure */
typedef struct {
    fat32_boot_t boot;
    u32 data_sector;
    u32 fat_sector;
    u32 cluster_size;
} fat32_fs_t;

/* FAT32 inode private data */
typedef struct {
    u32 first_cluster;
    u32 dir_sector;
    u32 dir_offset;
} fat32_inode_t;

/* Mount FAT32 filesystem */
static int fat32_mount(const char *device, const char *mount_point)
{
    fat32_fs_t *fs;
    fat32_boot_t *boot;
    block_dev_t *dev;
    u16 boot_sig;
    u32 fat_sectors;
    extern void *kmalloc(unsigned long size);
    extern void kfree(void *ptr);

    early_puts("FAT32: Mounting ");
    early_puts(device);
    early_puts(" on ");
    early_puts(mount_point);
    early_puts("\n");

    /* Find block device */
    dev = blockdev_find(device);
    if (dev == NULL) {
        early_puts("FAT32: Block device not found: ");
        early_puts(device);
        early_puts("\n");
        return -1;
    }

    /* Allocate filesystem structure */
    fs = (fat32_fs_t *)kmalloc(sizeof(fat32_fs_t));
    if (fs == NULL) {
        early_puts("FAT32: Failed to allocate filesystem structure\n");
        return -1;
    }

    /* Allocate buffer for boot sector */
    boot = (fat32_boot_t *)kmalloc(sizeof(fat32_boot_t));
    if (boot == NULL) {
        early_puts("FAT32: Failed to allocate boot sector buffer\n");
        kfree(fs);
        return -1;
    }

    /* Read boot sector from sector 0 */
    if (blockdev_read(dev, 0, (void *)boot, 1) < 0) {
        early_puts("FAT32: Failed to read boot sector\n");
        kfree(boot);
        kfree(fs);
        return -1;
    }

    /* Validate boot sector signature at offset 510 (0xAA55) */
    /* Note: The signature is stored as two bytes at offset 510-511 */
    u8 *boot_ptr = (u8 *)boot;
    boot_sig = (u16)(boot_ptr[510] | (boot_ptr[511] << 8));
    if (boot_sig != FAT32_BOOT_SIGNATURE) {
        early_puts("FAT32: Invalid boot sector signature\n");
        kfree(boot);
        kfree(fs);
        return -1;
    }

    /* Validate media type (should be 0xF0 or 0xF8-0xFF) */
    if (boot->media != 0xF0 && boot->media < 0xF8) {
        early_puts("FAT32: Invalid media type\n");
        kfree(boot);
        kfree(fs);
        return -1;
    }

    /* Validate FAT32 specific fields */
    if (boot->fat_size_16 != 0) {
        early_puts("FAT32: Not FAT32 (fat_size_16 != 0)\n");
        kfree(boot);
        kfree(fs);
        return -1;
    }

    if (boot->fat_size_32 == 0) {
        early_puts("FAT32: Invalid FAT size\n");
        kfree(boot);
        kfree(fs);
        return -1;
    }

    if (boot->root_cluster < 2) {
        early_puts("FAT32: Invalid root cluster\n");
        kfree(boot);
        kfree(fs);
        return -1;
    }

    /* Calculate cluster size */
    fs->cluster_size = boot->sectors_per_cluster * boot->bytes_per_sector;
    if (fs->cluster_size == 0 || fs->cluster_size > 32768) {
        early_puts("FAT32: Invalid cluster size\n");
        kfree(boot);
        kfree(fs);
        return -1;
    }

    /* Calculate data sector and FAT sector */
    fat_sectors = boot->fat_size_32;
    fs->fat_sector = boot->reserved_sectors;
    fs->data_sector = boot->reserved_sectors + (boot->num_fats * fat_sectors);

    /* Copy boot sector */
    fs->boot = *boot;

    /* Success */
    early_puts("FAT32: Mounted successfully\n");
    early_puts("FAT32: Cluster size: ");
    /* TODO: print cluster_size */
    early_puts(" bytes\n");

    kfree(boot);
    return 0;
}

/* Unmount FAT32 filesystem */
static int fat32_unmount(const char *mount_point)
{
    early_puts("FAT32: Unmounting ");
    early_puts(mount_point);
    early_puts("\n");

    return 0;
}

/* Read inode from FAT32 */
static inode_t *fat32_read_inode(u64 ino)
{
    (void)ino;
    /* TODO: Implement inode reading */
    return NULL;
}

/* Write inode to FAT32 */
static int fat32_write_inode(inode_t *inode)
{
    (void)inode;
    /* TODO: Implement inode writing */
    return -1;
}

/* Delete FAT32 inode */
static int fat32_delete_inode(inode_t *inode)
{
    (void)inode;
    /* TODO: Implement inode deletion */
    return -1;
}

/* Open file on FAT32 */
static int fat32_open(inode_t *inode, file_t *file, int flags)
{
    (void)inode;
    (void)file;
    (void)flags;
    /* TODO: Implement file open */
    return -1;
}

/* Close file on FAT32 */
static int fat32_close(file_t *file)
{
    (void)file;
    /* TODO: Implement file close */
    return 0;
}

/* Read from FAT32 file */
static ssize_t fat32_read(file_t *file, void *buf, size_t count)
{
    (void)file;
    (void)buf;
    (void)count;
    /* TODO: Implement file read */
    return -1;
}

/* Write to FAT32 file */
static ssize_t fat32_write(file_t *file, const void *buf, size_t count)
{
    (void)file;
    (void)buf;
    (void)count;
    /* TODO: Implement file write */
    return -1;
}

/* Seek in FAT32 file */
static int fat32_seek(file_t *file, s64 offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    /* TODO: Implement file seek */
    return -1;
}

/* Create directory on FAT32 */
static int fat32_mkdir(inode_t *parent, const char *name, u32 mode)
{
    (void)parent;
    (void)name;
    (void)mode;
    /* TODO: Implement mkdir */
    return -1;
}

/* Remove directory on FAT32 */
static int fat32_rmdir(inode_t *parent, const char *name)
{
    (void)parent;
    (void)name;
    /* TODO: Implement rmdir */
    return -1;
}

/* Read directory on FAT32 */
static int fat32_readdir(inode_t *dir, dirent_t *entries, int count)
{
    (void)dir;
    (void)entries;
    (void)count;
    /* TODO: Implement readdir */
    return 0;
}

/* Lookup file on FAT32 */
static inode_t *fat32_lookup(inode_t *dir, const char *name)
{
    (void)dir;
    (void)name;
    /* TODO: Implement file lookup */
    return NULL;
}

/* FAT32 filesystem operations */
static fs_ops_t fat32_ops = {
    .mount = fat32_mount,
    .unmount = fat32_unmount,
    .read_inode = fat32_read_inode,
    .write_inode = fat32_write_inode,
    .delete_inode = fat32_delete_inode,
    .open = fat32_open,
    .close = fat32_close,
    .read = fat32_read,
    .write = fat32_write,
    .seek = fat32_seek,
    .mkdir = fat32_mkdir,
    .rmdir = fat32_rmdir,
    .readdir = fat32_readdir,
    .lookup = fat32_lookup,
    .name = "fat32"
};

/* Register FAT32 filesystem */
int fat32_init(void)
{
    extern int vfs_register_fs(const char *name, fs_ops_t *ops);

    return vfs_register_fs("fat32", &fat32_ops);
}
