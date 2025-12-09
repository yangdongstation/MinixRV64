/* FAT12/FAT16 Filesystem Implementation */

#include <minix/config.h>
#include <types.h>
#include <minix/vfs.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* FAT Boot Sector for FAT12/FAT16 */
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
    u8 drive_number;
    u8 reserved;
    u8 boot_signature;
    u32 volume_id;
    u8 volume_label[11];
    u8 fs_type[8];
} fat_boot_t;

/* FAT Directory Entry */
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
} fat_dirent_t;

/* File attributes */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME     0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN        0x0F

/* FAT filesystem structure */
typedef struct {
    fat_boot_t boot;
    u32 data_sector;
    u32 fat_sector;
    u32 cluster_size;
    u32 fat_type;  /* 12 or 16 */
} fat_fs_t;

/* FAT inode private data */
typedef struct {
    u32 first_cluster;
    u32 dir_sector;
    u32 dir_offset;
} fat_inode_t;

/* Mount FAT filesystem */
static int fat_mount(const char *device, const char *mount_point)
{
    early_puts("FAT: Mounting ");
    early_puts(device);
    early_puts(" on ");
    early_puts(mount_point);
    early_puts("\n");

    /* TODO: Read boot sector and validate FAT signature */
    /* TODO: Determine FAT type (12 or 16) */
    /* TODO: Initialize FAT metadata */
    return 0;
}

/* Unmount FAT filesystem */
static int fat_unmount(const char *mount_point)
{
    early_puts("FAT: Unmounting ");
    early_puts(mount_point);
    early_puts("\n");

    return 0;
}

/* Read inode from FAT */
static inode_t *fat_read_inode(u64 ino)
{
    (void)ino;
    /* TODO: Implement inode reading */
    return NULL;
}

/* Write inode to FAT */
static int fat_write_inode(inode_t *inode)
{
    (void)inode;
    /* TODO: Implement inode writing */
    return -1;
}

/* Delete FAT inode */
static int fat_delete_inode(inode_t *inode)
{
    (void)inode;
    /* TODO: Implement inode deletion */
    return -1;
}

/* Open file on FAT */
static int fat_open(inode_t *inode, file_t *file, int flags)
{
    (void)inode;
    (void)file;
    (void)flags;
    /* TODO: Implement file open */
    return -1;
}

/* Close file on FAT */
static int fat_close(file_t *file)
{
    (void)file;
    /* TODO: Implement file close */
    return 0;
}

/* Read from FAT file */
static ssize_t fat_read(file_t *file, void *buf, size_t count)
{
    (void)file;
    (void)buf;
    (void)count;
    /* TODO: Implement file read */
    return -1;
}

/* Write to FAT file */
static ssize_t fat_write(file_t *file, const void *buf, size_t count)
{
    (void)file;
    (void)buf;
    (void)count;
    /* TODO: Implement file write */
    return -1;
}

/* Seek in FAT file */
static int fat_seek(file_t *file, s64 offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    /* TODO: Implement file seek */
    return -1;
}

/* Create directory on FAT */
static int fat_mkdir(inode_t *parent, const char *name, u32 mode)
{
    (void)parent;
    (void)name;
    (void)mode;
    /* TODO: Implement mkdir */
    return -1;
}

/* Remove directory on FAT */
static int fat_rmdir(inode_t *parent, const char *name)
{
    (void)parent;
    (void)name;
    /* TODO: Implement rmdir */
    return -1;
}

/* Read directory on FAT */
static int fat_readdir(inode_t *dir, dirent_t *entries, int count)
{
    (void)dir;
    (void)entries;
    (void)count;
    /* TODO: Implement readdir */
    return 0;
}

/* Lookup file on FAT */
static inode_t *fat_lookup(inode_t *dir, const char *name)
{
    (void)dir;
    (void)name;
    /* TODO: Implement file lookup */
    return NULL;
}

/* FAT filesystem operations */
static fs_ops_t fat_ops = {
    .mount = fat_mount,
    .unmount = fat_unmount,
    .read_inode = fat_read_inode,
    .write_inode = fat_write_inode,
    .delete_inode = fat_delete_inode,
    .open = fat_open,
    .close = fat_close,
    .read = fat_read,
    .write = fat_write,
    .seek = fat_seek,
    .mkdir = fat_mkdir,
    .rmdir = fat_rmdir,
    .readdir = fat_readdir,
    .lookup = fat_lookup,
    .name = "fat"
};

/* Register FAT filesystem */
int fat_init(void)
{
    extern int vfs_register_fs(const char *name, fs_ops_t *ops);

    return vfs_register_fs("fat", &fat_ops);
}
