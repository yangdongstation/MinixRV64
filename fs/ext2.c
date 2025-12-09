/* EXT2 Filesystem Implementation */

#include <minix/config.h>
#include <types.h>
#include <minix/vfs.h>
#include <minix/blockdev.h>
#include <minix/blockdev_priv.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* EXT2 Superblock location and size */
#define EXT2_SUPERBLOCK_OFFSET  1024    /* 1KB offset from start */
#define EXT2_SUPERBLOCK_SIZE    1024    /* 1KB size */
#define EXT2_GROUP_DESC_OFFSET  2048    /* Group descriptors follow */

/* Block group descriptor */
typedef struct {
    u32 block_bitmap;           /* Block bitmap block */
    u32 inode_bitmap;           /* Inode bitmap block */
    u32 inode_table;            /* Inode table block */
    u16 free_blocks_count;      /* Free blocks in group */
    u16 free_inodes_count;      /* Free inodes in group */
    u16 used_dirs_count;        /* Used directories count */
    u16 pad;
    u32 reserved[3];
} ext2_group_desc_t;

/* EXT2 Superblock */
typedef struct {
    u32 total_inodes;
    u32 total_blocks;
    u32 reserved_blocks;
    u32 free_blocks;
    u32 free_inodes;
    u32 block_size_log2;        /* Log2(block size) - 10 */
    u32 fragment_size_log2;
    u32 blocks_per_group;
    u32 fragments_per_group;
    u32 inodes_per_group;
    u32 mount_time;
    u32 write_time;
    u16 mount_count;
    u16 max_mount_count;
    u16 magic;                  /* 0xEF53 */
    u16 state;
    u16 errors;
    u16 minor_revision;
    u32 last_check_time;
    u32 check_interval;
    u32 os_id;
    u32 major_revision;
    u16 uid_reserved;
    u16 gid_reserved;
    /* Extended fields */
    u32 first_inode;
    u16 inode_size;
    u16 block_group_number;
    u32 features_compatible;
    u32 features_incompatible;
    u32 features_read_only;
    u8 unique_id[16];
    u8 volume_name[16];
    u8 mount_path[64];
} ext2_superblock_t;

#define EXT2_MAGIC              0xEF53
#define EXT2_BLOCK_SIZE(sb)     (1024 << (sb)->block_size_log2)
#define EXT2_INODE_SIZE(sb)     ((sb)->inode_size ?: 128)

/* EXT2 Inode */
typedef struct {
    u16 mode;
    u16 uid;
    u32 size;
    u32 atime;
    u32 ctime;
    u32 mtime;
    u32 dtime;
    u16 gid;
    u16 links_count;
    u32 blocks_count;
    u32 flags;
    u32 osd1;
    u32 direct_blocks[12];
    u32 indirect_block;
    u32 doubly_indirect;
    u32 triply_indirect;
    u32 generation;
    u32 file_acl;
    u32 dir_acl;
    u32 fragment_addr;
    u32 osd2[3];
} ext2_inode_t;

/* EXT2 Directory Entry */
typedef struct {
    u32 inode;
    u16 rec_len;
    u8 name_len;
    u8 file_type;
    u8 name[256];
} ext2_dirent_t;

/* File types */
#define EXT2_FT_UNKNOWN     0
#define EXT2_FT_REG_FILE    1
#define EXT2_FT_DIR         2
#define EXT2_FT_CHRDEV      3
#define EXT2_FT_BLKDEV      4
#define EXT2_FT_FIFO        5
#define EXT2_FT_SOCK        6
#define EXT2_FT_SYMLINK     7

/* Inode modes */
#define EXT2_S_IFREG        0x8000
#define EXT2_S_IFDIR        0x4000
#define EXT2_S_IFLNK        0xA000

/* EXT2 filesystem structure */
typedef struct {
    ext2_superblock_t superblock;
    u32 block_size;
    u32 inode_size;
    u32 inodes_per_block;
} ext2_fs_t;

/* EXT2 inode private data */
typedef struct {
    u32 inode_num;
    ext2_inode_t inode_data;
} ext2_inode_t_private;

/* Mount EXT2 filesystem */
static int ext2_mount(const char *device, const char *mount_point)
{
    ext2_fs_t *fs;
    ext2_superblock_t *sb;
    block_dev_t *dev;
    extern void *kmalloc(unsigned long size);
    extern void kfree(void *ptr);

    early_puts("EXT2: Mounting ");
    early_puts(device);
    early_puts(" on ");
    early_puts(mount_point);
    early_puts("\n");

    /* Find block device */
    dev = blockdev_find(device);
    if (dev == NULL) {
        early_puts("EXT2: Block device not found: ");
        early_puts(device);
        early_puts("\n");
        return -1;
    }

    /* Allocate filesystem structure */
    fs = (ext2_fs_t *)kmalloc(sizeof(ext2_fs_t));
    if (fs == NULL) {
        early_puts("EXT2: Failed to allocate filesystem structure\n");
        return -1;
    }

    /* Allocate buffer for superblock */
    sb = (ext2_superblock_t *)kmalloc(sizeof(ext2_superblock_t));
    if (sb == NULL) {
        early_puts("EXT2: Failed to allocate superblock buffer\n");
        kfree(fs);
        return -1;
    }

    /* Read superblock from device (at offset 1024, typically in second 1K block) */
    if (blockdev_read(dev, 0, (void *)sb, 1) < 0) {
        early_puts("EXT2: Failed to read superblock\n");
        kfree(sb);
        kfree(fs);
        return -1;
    }

    /* Validate EXT2 magic number */
    if (sb->magic != EXT2_MAGIC) {
        early_puts("EXT2: Invalid superblock magic: ");
        /* TODO: print hex magic */
        early_puts("\n");
        kfree(sb);
        kfree(fs);
        return -1;
    }

    /* Calculate block size */
    fs->block_size = EXT2_BLOCK_SIZE(sb);
    if (fs->block_size < 1024 || fs->block_size > 65536) {
        early_puts("EXT2: Invalid block size\n");
        kfree(sb);
        kfree(fs);
        return -1;
    }

    /* Initialize inode size */
    fs->inode_size = EXT2_INODE_SIZE(sb);
    if (fs->inode_size < 128 || fs->inode_size > 4096) {
        early_puts("EXT2: Invalid inode size\n");
        kfree(sb);
        kfree(fs);
        return -1;
    }

    /* Calculate inodes per block */
    fs->inodes_per_block = fs->block_size / fs->inode_size;
    if (fs->inodes_per_block == 0) {
        early_puts("EXT2: Invalid inodes per block calculation\n");
        kfree(sb);
        kfree(fs);
        return -1;
    }

    /* Copy superblock data */
    fs->superblock = *sb;

    /* Success */
    early_puts("EXT2: Mounted successfully\n");
    early_puts("EXT2: Block size: ");
    /* TODO: print block_size */
    early_puts(" bytes\n");

    kfree(sb);
    return 0;
}

/* Unmount EXT2 filesystem */
static int ext2_unmount(const char *mount_point)
{
    early_puts("EXT2: Unmounting ");
    early_puts(mount_point);
    early_puts("\n");

    return 0;
}

/* Read inode from EXT2 */
static inode_t *ext2_read_inode(u64 ino)
{
    inode_t *inode;
    ext2_inode_t_private *priv;
    extern void *kmalloc(unsigned long size);

    if (ino == 0) {
        return NULL;  /* Invalid inode number */
    }

    /* Allocate inode structure */
    inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (inode == NULL) {
        return NULL;
    }

    /* Allocate private data */
    priv = (ext2_inode_t_private *)kmalloc(sizeof(ext2_inode_t_private));
    if (priv == NULL) {
        extern void kfree(void *ptr);
        kfree(inode);
        return NULL;
    }

    /* TODO: Implement full inode reading from inode table */
    /* For now, initialize with basic values */
    inode->ino = ino;
    inode->mode = 0;
    inode->nlink = 0;
    inode->uid = 0;
    inode->gid = 0;
    inode->size = 0;
    inode->atime = 0;
    inode->mtime = 0;
    inode->ctime = 0;
    inode->blksize = 4096;
    inode->blocks = 0;
    inode->fs_private = (void *)priv;
    inode->parent = NULL;
    inode->next = NULL;

    priv->inode_num = ino;

    return inode;
}

/* Write inode to EXT2 */
static int ext2_write_inode(inode_t *inode)
{
    (void)inode;
    /* TODO: Implement inode writing */
    return -1;
}

/* Delete EXT2 inode */
static int ext2_delete_inode(inode_t *inode)
{
    (void)inode;
    /* TODO: Implement inode deletion */
    return -1;
}

/* Open file on EXT2 */
static int ext2_open(inode_t *inode, file_t *file, int flags)
{
    (void)inode;
    (void)file;
    (void)flags;
    /* TODO: Implement file open */
    return -1;
}

/* Close file on EXT2 */
static int ext2_close(file_t *file)
{
    (void)file;
    /* TODO: Implement file close */
    return 0;
}

/* Read from EXT2 file */
static ssize_t ext2_read(file_t *file, void *buf, size_t count)
{
    (void)file;
    (void)buf;
    (void)count;
    /* TODO: Implement file read */
    return -1;
}

/* Write to EXT2 file */
static ssize_t ext2_write(file_t *file, const void *buf, size_t count)
{
    (void)file;
    (void)buf;
    (void)count;
    /* TODO: Implement file write */
    return -1;
}

/* Seek in EXT2 file */
static int ext2_seek(file_t *file, s64 offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    /* TODO: Implement file seek */
    return -1;
}

/* Create directory on EXT2 */
static int ext2_mkdir(inode_t *parent, const char *name, u32 mode)
{
    (void)parent;
    (void)name;
    (void)mode;
    /* TODO: Implement mkdir */
    return -1;
}

/* Remove directory on EXT2 */
static int ext2_rmdir(inode_t *parent, const char *name)
{
    (void)parent;
    (void)name;
    /* TODO: Implement rmdir */
    return -1;
}

/* Read directory on EXT2 */
static int ext2_readdir(inode_t *dir, dirent_t *entries, int count)
{
    (void)dir;
    (void)entries;
    (void)count;
    /* TODO: Implement readdir */
    return 0;
}

/* Lookup file on EXT2 */
static inode_t *ext2_lookup(inode_t *dir, const char *name)
{
    (void)dir;
    (void)name;
    /* TODO: Implement file lookup */
    return NULL;
}

/* EXT2 filesystem operations */
static fs_ops_t ext2_ops = {
    .mount = ext2_mount,
    .unmount = ext2_unmount,
    .read_inode = ext2_read_inode,
    .write_inode = ext2_write_inode,
    .delete_inode = ext2_delete_inode,
    .open = ext2_open,
    .close = ext2_close,
    .read = ext2_read,
    .write = ext2_write,
    .seek = ext2_seek,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
    .readdir = ext2_readdir,
    .lookup = ext2_lookup,
    .name = "ext2"
};

/* Register EXT2 filesystem */
int ext2_init(void)
{
    extern int vfs_register_fs(const char *name, fs_ops_t *ops);

    return vfs_register_fs("ext2", &ext2_ops);
}
