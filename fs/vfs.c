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
    mnt->root = NULL;

    /* Call filesystem mount operation */
    if (ops->mount && ops->mount(device, mount_point) != 0) {
        early_puts("VFS: Failed to mount ");
        early_puts(fstype);
        early_puts("\n");
        return -1;
    }

    /* Get root inode from filesystem */
    early_puts("VFS: Getting root inode...\n");
    if (ops->read_inode) {
        mnt->root = ops->read_inode(1);  /* Root inode is always 1 */
        if (mnt->root == NULL) {
            early_puts("VFS: Failed to get root inode\n");
            return -1;
        }
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

/* Parse path component */
static const char *parse_path_component(const char *path, char *component, int max_len)
{
    int i = 0;

    /* Skip leading slashes */
    while (*path == '/') {
        path++;
    }

    /* Copy component until next slash or end */
    while (*path && *path != '/' && i < max_len - 1) {
        component[i++] = *path++;
    }
    component[i] = '\0';

    return path;
}

/* Look up path in filesystem */
inode_t *vfs_lookup_path(const char *path)
{
    mount_point_t *mnt;
    inode_t *current_inode;
    char component[256];
    const char *p;

    if (path == NULL || *path == '\0') {
        return NULL;
    }

    /* Find mount point for path */
    mnt = vfs_find_mount(path);
    if (mnt == NULL) {
        early_puts("VFS: Mount point not found for path: ");
        early_puts(path);
        early_puts("\n");
        return NULL;
    }

    /* Start from root inode of mount point */
    current_inode = mnt->root;
    if (current_inode == NULL) {
        return NULL;
    }

    /* Skip mount point prefix */
    p = path;
    const char *mnt_path = mnt->path;
    while (*mnt_path && *p && *mnt_path == *p) {
        mnt_path++;
        p++;
    }

    /* If path is just the mount point, return root inode */
    if (*p == '\0' || (*p == '/' && *(p+1) == '\0')) {
        return current_inode;
    }

    /* Parse and lookup each path component */
    while (*p != '\0') {
        p = parse_path_component(p, component, sizeof(component));

        if (component[0] == '\0') {
            break;  /* End of path */
        }

        /* Skip "." (current directory) */
        if (component[0] == '.' && component[1] == '\0') {
            continue;
        }

        /* Handle ".." (parent directory) */
        if (component[0] == '.' && component[1] == '.' && component[2] == '\0') {
            if (current_inode->parent) {
                current_inode = current_inode->parent;
            }
            continue;
        }

        /* Lookup component in current directory */
        if (mnt->ops->lookup == NULL) {
            return NULL;
        }

        current_inode = mnt->ops->lookup(current_inode, component);
        if (current_inode == NULL) {
            return NULL;  /* Component not found */
        }
    }

    return current_inode;
}

/* Open a file */
file_t *vfs_open(const char *path, int flags)
{
    inode_t *inode;
    file_t *file;
    mount_point_t *mnt;

    inode = vfs_lookup_path(path);

    /* If file doesn't exist and O_CREAT is set, create it */
    if (inode == NULL && (flags & O_CREAT)) {
        char parent_path[256];
        char file_name[256];
        const char *p, *last_slash;
        char *q;
        inode_t *parent_inode;

        mnt = vfs_find_mount(path);
        if (mnt == NULL) {
            return NULL;
        }

        /* Find parent directory path and file name */
        last_slash = NULL;
        for (p = path; *p; p++) {
            if (*p == '/') {
                last_slash = p;
            }
        }

        if (last_slash == NULL || last_slash == path) {
            /* No parent or root parent */
            parent_inode = mnt->root;
            p = path;
            while (*p == '/') p++;
            q = file_name;
            while (*p && q < file_name + sizeof(file_name) - 1) {
                *q++ = *p++;
            }
            *q = '\0';
        } else {
            /* Copy parent path */
            p = path;
            q = parent_path;
            while (p < last_slash && q < parent_path + sizeof(parent_path) - 1) {
                *q++ = *p++;
            }
            *q = '\0';

            /* Copy file name */
            p = last_slash + 1;
            q = file_name;
            while (*p && q < file_name + sizeof(file_name) - 1) {
                *q++ = *p++;
            }
            *q = '\0';

            /* Lookup parent directory */
            parent_inode = vfs_lookup_path(parent_path);
            if (parent_inode == NULL) {
                return NULL;
            }
        }

        /* Create file using mkdir with S_IFREG (this is a hack, ideally we'd have create op) */
        if (mnt->ops->mkdir) {
            /* Use mkdir to create a regular file */
            if (mnt->ops->mkdir(parent_inode, file_name, 0644) < 0) {
                return NULL;
            }
            /* Now lookup the created file */
            inode = vfs_lookup_path(path);
            if (inode) {
                /* Change mode to regular file */
                inode->mode = (inode->mode & ~S_IFMT) | S_IFREG;
            }
        }
    }

    if (inode == NULL) {
        return NULL;
    }

    /* Allocate file structure */
    file = (file_t *)kmalloc(sizeof(file_t));
    if (file == NULL) {
        return NULL;
    }

    file->inode = inode;
    file->pos = (flags & O_APPEND) ? inode->size : 0;
    file->flags = flags;
    file->private = NULL;

    /* Truncate if requested */
    if (flags & O_TRUNC) {
        inode->size = 0;
        /* Update filesystem-specific size */
        if (inode->fs_private) {
            mnt = vfs_find_mount("");
            if (mnt && mnt->ops->write_inode) {
                mnt->ops->write_inode(inode);
            }
        }
    }

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

    /* Use root path to find mount - all files are under some mount point */
    mount_point_t *mnt = vfs_find_mount("/");
    if (mnt == NULL || mnt->ops->read == NULL) {
        return -1;
    }

    return mnt->ops->read(file, buf, count);
}

/* Write to file */
ssize_t vfs_write(file_t *file, const void *buf, size_t count)
{
    early_puts("[vfs_write] Starting\n");

    if (file == NULL || file->inode == NULL) {
        early_puts("[vfs_write] NULL file or inode\n");
        return -1;
    }

    if (file->inode->fs_private == NULL) {
        early_puts("[vfs_write] NULL fs_private\n");
        return -1;
    }

    early_puts("[vfs_write] Finding mount\n");
    /* Use root path to find mount - all files are under some mount point */
    mount_point_t *mnt = vfs_find_mount("/");
    if (mnt == NULL || mnt->ops->write == NULL) {
        early_puts("[vfs_write] No mount or write op\n");
        return -1;
    }

    early_puts("[vfs_write] Calling ramfs_write\n");
    ssize_t result = mnt->ops->write(file, buf, count);
    early_puts("[vfs_write] Result: ");
    early_puthex(result);
    early_puts("\n");
    return result;
}

/* Make directory */
int vfs_mkdir(const char *path, u32 mode)
{
    mount_point_t *mnt;
    char parent_path[256];
    char dir_name[256];
    const char *p, *last_slash;
    char *q;
    inode_t *parent_inode;
    int result;

    if (path == NULL) {
        return -1;
    }

    early_puts("[vfs_mkdir] Starting mkdir for: ");
    early_puts(path);
    early_puts("\n");

    mnt = vfs_find_mount(path);
    if (mnt == NULL || mnt->ops->mkdir == NULL) {
        early_puts("[vfs_mkdir] No mount or mkdir op\n");
        return -1;
    }

    early_puts("[vfs_mkdir] Mount found\n");

    /* Find parent directory path and directory name */
    last_slash = NULL;
    for (p = path; *p; p++) {
        if (*p == '/') {
            last_slash = p;
        }
    }

    if (last_slash == NULL || last_slash == path) {
        /* No parent or root parent */
        early_puts("[vfs_mkdir] Using root as parent\n");
        parent_inode = mnt->root;
        p = path;
        while (*p == '/') p++;
        q = dir_name;
        while (*p && q < dir_name + sizeof(dir_name) - 1) {
            *q++ = *p++;
        }
        *q = '\0';
    } else {
        /* Copy parent path */
        early_puts("[vfs_mkdir] Looking up parent\n");
        p = path;
        q = parent_path;
        while (p < last_slash && q < parent_path + sizeof(parent_path) - 1) {
            *q++ = *p++;
        }
        *q = '\0';

        /* Copy directory name */
        p = last_slash + 1;
        q = dir_name;
        while (*p && q < dir_name + sizeof(dir_name) - 1) {
            *q++ = *p++;
        }
        *q = '\0';

        /* Lookup parent directory */
        early_puts("[vfs_mkdir] Calling vfs_lookup_path for: ");
        early_puts(parent_path);
        early_puts("\n");
        parent_inode = vfs_lookup_path(parent_path);
        if (parent_inode == NULL) {
            early_puts("[vfs_mkdir] Parent not found\n");
            return -1;
        }
    }

    early_puts("[vfs_mkdir] Calling ramfs mkdir for: ");
    early_puts(dir_name);
    early_puts("\n");
    result = mnt->ops->mkdir(parent_inode, dir_name, mode);
    early_puts("[vfs_mkdir] mkdir returned: ");
    if (result == 0) {
        early_puts("SUCCESS\n");
    } else {
        early_puts("FAILED\n");
    }
    return result;
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

/* Read directory entries */
int vfs_readdir(const char *path, dirent_t *entries, int count)
{
    inode_t *inode;
    mount_point_t *mnt;
    int result;

    early_puts("[vfs_readdir] path: ");
    early_puts(path);
    early_puts("\n");

    if (path == NULL || entries == NULL) {
        early_puts("[vfs_readdir] NULL params\n");
        return -1;
    }

    inode = vfs_lookup_path(path);
    if (inode == NULL) {
        early_puts("[vfs_readdir] inode not found\n");
        return -1;
    }

    early_puts("[vfs_readdir] inode found, ino=");
    early_puthex(inode->ino);
    early_puts("\n");

    mnt = vfs_find_mount(path);
    if (mnt == NULL || mnt->ops->readdir == NULL) {
        early_puts("[vfs_readdir] no mount or readdir\n");
        return -1;
    }

    early_puts("[vfs_readdir] calling ramfs_readdir\n");
    result = mnt->ops->readdir(inode, entries, count);
    early_puts("[vfs_readdir] result=");
    early_puthex(result);
    early_puts("\n");

    return result;
}

/* Create a new file */
int vfs_create(const char *path, u32 mode)
{
    mount_point_t *mnt;
    char parent_path[256];
    char file_name[256];
    const char *p, *last_slash;
    char *q;
    inode_t *parent_inode;

    if (path == NULL) {
        return -1;
    }

    mnt = vfs_find_mount(path);
    if (mnt == NULL) {
        return -1;
    }

    /* Find parent directory path and file name */
    last_slash = NULL;
    for (p = path; *p; p++) {
        if (*p == '/') {
            last_slash = p;
        }
    }

    if (last_slash == NULL || last_slash == path) {
        /* No parent or root parent */
        parent_inode = mnt->root;
        p = path;
        while (*p == '/') p++;
        q = file_name;
        while (*p && q < file_name + sizeof(file_name) - 1) {
            *q++ = *p++;
        }
        *q = '\0';
    } else {
        /* Copy parent path */
        p = path;
        q = parent_path;
        while (p < last_slash && q < parent_path + sizeof(parent_path) - 1) {
            *q++ = *p++;
        }
        *q = '\0';

        /* Copy file name */
        p = last_slash + 1;
        q = file_name;
        while (*p && q < file_name + sizeof(file_name) - 1) {
            *q++ = *p++;
        }
        *q = '\0';

        /* Lookup parent directory */
        parent_inode = vfs_lookup_path(parent_path);
        if (parent_inode == NULL) {
            return -1;
        }
    }

    /* Use mkdir with S_IFREG mode for file creation */
    /* This is a temporary solution - ideally we'd have a separate create operation */
    (void)parent_inode;
    (void)file_name;
    (void)mode;

    return -1;  /* Not fully implemented yet */
}
