/* Virtual File System Implementation */

#include <minix/config.h>
#include <types.h>
#include <minix/vfs.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of filesystem types */
#define MAX_FS_TYPES    16
/* Maximum mount points */
#define MAX_MOUNT_POINTS 32
/* Inode hash table size */
#define INODE_HASH_SIZE 256

/* Registered filesystems */
static fs_ops_t *fs_types[MAX_FS_TYPES];
static int num_fs_types = 0;

/* Mount point structure */
typedef struct mount_point {
    char path[256];
    char device[256];
    const char *fstype;
    inode_t *root;
    fs_ops_t *ops;
} mount_point_t;

/* Global mount table */
static mount_point_t mount_table[MAX_MOUNT_POINTS];
static int num_mounts = 0;

/* Inode cache */
static inode_t *inode_cache[INODE_HASH_SIZE];

/* Forward declarations */
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);

/* Helper function to hash inode number */
static unsigned int __attribute__((unused)) inode_hash(u64 ino)
{
    return ino % INODE_HASH_SIZE;
}

/* Initialize VFS */
int vfs_init(void)
{
    int i;

    early_puts("✓ VFS ready\n");

    /* Initialize filesystem type array */
    for (i = 0; i < MAX_FS_TYPES; i++) {
        fs_types[i] = NULL;
    }

    /* Initialize inode cache */
    for (i = 0; i < INODE_HASH_SIZE; i++) {
        inode_cache[i] = NULL;
    }

    return 0;
}

/* Register a filesystem */
int vfs_register_fs(const char *name, fs_ops_t *ops)
{
    (void)name;  /* Suppress unused parameter warning */

    if (num_fs_types >= MAX_FS_TYPES) {
        return -1;
    }

    if (ops == NULL) {
        return -1;
    }

    fs_types[num_fs_types] = ops;
    num_fs_types++;

    return 0;
}

/* Find filesystem by type */
static fs_ops_t *vfs_find_fs(const char *fstype)
{
    int i;

    for (i = 0; i < num_fs_types; i++) {
        if (fs_types[i] && fs_types[i]->name) {
            const char *a = fs_types[i]->name;
            const char *b = fstype;

            /* Compare strings */
            while (*a && *b && *a == *b) {
                a++;
                b++;
            }

            if (*a == '\0' && *b == '\0') {
                return fs_types[i];
            }
        }
    }

    return NULL;
}

/* Mount a filesystem */
int vfs_mount(const char *device, const char *mount_point, const char *fstype)
{
    fs_ops_t *ops;
    mount_point_t *mnt;

    if (num_mounts >= MAX_MOUNT_POINTS) {
        early_puts("VFS: Too many mount points\n");
        return -1;
    }

    ops = vfs_find_fs(fstype);
    if (ops == NULL) {
        early_puts("VFS: Unknown filesystem type: ");
        early_puts(fstype);
        early_puts("\n");
        return -1;
    }

    mnt = &mount_table[num_mounts];

    /* Copy mount information */
    const char *p = device;
    char *q = mnt->device;
    while (*p && q < mnt->device + sizeof(mnt->device) - 1) {
        *q++ = *p++;
    }
    *q = '\0';

    p = mount_point;
    q = mnt->path;
    while (*p && q < mnt->path + sizeof(mnt->path) - 1) {
        *q++ = *p++;
    }
    *q = '\0';

    mnt->fstype = fstype;
    mnt->ops = ops;

    /* Call filesystem mount operation */
    if (ops->mount && ops->mount(device, mount_point) != 0) {
        early_puts("VFS: Failed to mount ");
        early_puts(fstype);
        early_puts("\n");
        return -1;
    }

    early_puts("VFS: Mounted ");
    early_puts(fstype);
    early_puts(" on ");
    early_puts(mount_point);
    early_puts("\n");

    num_mounts++;
    return 0;
}

/* Unmount a filesystem */
int vfs_unmount(const char *mount_point)
{
    int i;
    mount_point_t *mnt;

    for (i = 0; i < num_mounts; i++) {
        mnt = &mount_table[i];

        /* Compare mount paths */
        const char *a = mnt->path;
        const char *b = mount_point;
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }

        if (*a == '\0' && *b == '\0') {
            /* Found matching mount point */
            if (mnt->ops->unmount) {
                mnt->ops->unmount(mount_point);
            }

            /* Remove from mount table */
            for (int j = i; j < num_mounts - 1; j++) {
                mount_table[j] = mount_table[j + 1];
            }
            num_mounts--;

            return 0;
        }
    }

    return -1;  /* Mount point not found */
}

/* Find mount point for path */
static mount_point_t *vfs_find_mount(const char *path)
{
    int i, best_match = -1, best_len = 0;

    for (i = 0; i < num_mounts; i++) {
        const char *mnt_path = mount_table[i].path;
        const char *p = path;

        int len = 0;
        while (*mnt_path && *p && *mnt_path == *p) {
            mnt_path++;
            p++;
            len++;
        }

        if (*mnt_path == '\0' && len > best_len) {
            best_match = i;
            best_len = len;
        }
    }

    if (best_match >= 0) {
        return &mount_table[best_match];
    }

    return NULL;
}

/* Look up path in filesystem */
inode_t *vfs_lookup_path(const char *path)
{
    mount_point_t *mnt;

    if (path == NULL) {
        return NULL;
    }

    mnt = vfs_find_mount(path);
    if (mnt == NULL) {
        early_puts("VFS: Mount point not found for path: ");
        early_puts(path);
        early_puts("\n");
        return NULL;
    }

    if (mnt->ops->lookup == NULL) {
        return NULL;
    }

    /* TODO: Handle relative path lookup */
    return mnt->ops->lookup(mnt->root, path);
}

/* Open a file */
file_t *vfs_open(const char *path, int flags)
{
    inode_t *inode;
    file_t *file;

    inode = vfs_lookup_path(path);
    if (inode == NULL) {
        return NULL;
    }

    /* Allocate file structure */
    file = (file_t *)kmalloc(sizeof(file_t));
    if (file == NULL) {
        return NULL;
    }

    file->inode = inode;
    file->pos = 0;
    file->flags = flags;
    file->private = NULL;

    return file;
}

/* Close a file */
int vfs_close(file_t *file)
{
    if (file == NULL) {
        return -1;
    }

    if (file->inode && file->inode->fs_private) {
        /* Call filesystem-specific close if available */
    }

    kfree(file);
    return 0;
}

/* Read from file */
ssize_t vfs_read(file_t *file, void *buf, size_t count)
{
    if (file == NULL || file->inode == NULL) {
        return -1;
    }

    if (file->inode->fs_private == NULL) {
        return -1;
    }

    mount_point_t *mnt = vfs_find_mount("");
    if (mnt == NULL || mnt->ops->read == NULL) {
        return -1;
    }

    return mnt->ops->read(file, buf, count);
}

/* Write to file */
ssize_t vfs_write(file_t *file, const void *buf, size_t count)
{
    if (file == NULL || file->inode == NULL) {
        return -1;
    }

    if (file->inode->fs_private == NULL) {
        return -1;
    }

    mount_point_t *mnt = vfs_find_mount("");
    if (mnt == NULL || mnt->ops->write == NULL) {
        return -1;
    }

    return mnt->ops->write(file, buf, count);
}

/* Make directory */
int vfs_mkdir(const char *path, u32 mode)
{
    mount_point_t *mnt;

    (void)mode;  /* Suppress unused parameter warning */

    if (path == NULL) {
        return -1;
    }

    mnt = vfs_find_mount(path);
    if (mnt == NULL || mnt->ops->mkdir == NULL) {
        return -1;
    }

    /* TODO: Find parent directory */
    return -1;
}

/* Remove directory */
int vfs_rmdir(const char *path)
{
    mount_point_t *mnt;

    if (path == NULL) {
        return -1;
    }

    mnt = vfs_find_mount(path);
    if (mnt == NULL || mnt->ops->rmdir == NULL) {
        return -1;
    }

    /* TODO: Implement rmdir */
    return -1;
}
