/* RAM Filesystem (ramfs) Implementation */

#include <minix/config.h>
#include <types.h>
#include <minix/vfs.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of files in ramfs */
#define RAMFS_MAX_FILES     256
#define RAMFS_MAX_FILE_SIZE (1024 * 1024)  /* 1MB per file */

/* ramfs file structure */
typedef struct ramfs_file {
    char name[64];
    u64 ino;
    u32 mode;
    u32 size;
    void *data;             /* File data buffer */
    u32 capacity;           /* Allocated size */
    inode_t *inode;
    struct ramfs_file *parent;
    struct ramfs_file *next;
} ramfs_file_t;

/* ramfs state */
static struct {
    ramfs_file_t files[RAMFS_MAX_FILES];
    int file_count;
    inode_t *root_inode;
    u64 next_ino;
} ramfs_state;

/* Forward declarations */
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);
extern void *memcpy(void *dest, const void *src, unsigned long n);
extern void *memset(void *s, int c, unsigned long n);

/* Find file by name in parent directory */
static ramfs_file_t *ramfs_find_file(ramfs_file_t *parent, const char *name)
{
    int i;
    const char *a, *b;

    for (i = 0; i < ramfs_state.file_count; i++) {
        if (ramfs_state.files[i].parent == parent) {
            a = ramfs_state.files[i].name;
            b = name;

            /* Compare strings */
            while (*a && *b && *a == *b) {
                a++;
                b++;
            }

            if (*a == '\0' && *b == '\0') {
                return &ramfs_state.files[i];
            }
        }
    }

    return NULL;
}

/* Find file by inode number */
static ramfs_file_t *ramfs_find_by_ino(u64 ino)
{
    int i;

    for (i = 0; i < ramfs_state.file_count; i++) {
        if (ramfs_state.files[i].ino == ino) {
            return &ramfs_state.files[i];
        }
    }

    return NULL;
}

/* Mount ramfs */
static int ramfs_mount(const char *device, const char *mount_point)
{
    (void)device;

    early_puts("ramfs: Mounting on ");
    early_puts(mount_point);
    early_puts("\n");

    /* Initialize state */
    ramfs_state.file_count = 0;
    ramfs_state.next_ino = 2;  /* Start from 2 (1 is root) */

    /* Create root inode */
    ramfs_state.root_inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (ramfs_state.root_inode == NULL) {
        return -1;
    }

    ramfs_state.root_inode->ino = 1;
    ramfs_state.root_inode->mode = S_IFDIR | 0755;
    ramfs_state.root_inode->nlink = 2;
    ramfs_state.root_inode->uid = 0;
    ramfs_state.root_inode->gid = 0;
    ramfs_state.root_inode->size = 0;
    ramfs_state.root_inode->parent = NULL;
    ramfs_state.root_inode->next = NULL;
    ramfs_state.root_inode->fs_private = NULL;

    early_puts("ramfs: Mounted successfully\n");
    return 0;
}

/* Unmount ramfs */
static int ramfs_unmount(const char *mount_point)
{
    int i;

    (void)mount_point;

    /* Free all file data */
    for (i = 0; i < ramfs_state.file_count; i++) {
        if (ramfs_state.files[i].data) {
            kfree(ramfs_state.files[i].data);
        }
        if (ramfs_state.files[i].inode) {
            kfree(ramfs_state.files[i].inode);
        }
    }

    if (ramfs_state.root_inode) {
        kfree(ramfs_state.root_inode);
        ramfs_state.root_inode = NULL;
    }

    return 0;
}

/* Read inode */
static inode_t *ramfs_read_inode(u64 ino)
{
    ramfs_file_t *file;

    if (ino == 1) {
        return ramfs_state.root_inode;
    }

    file = ramfs_find_by_ino(ino);
    if (file == NULL) {
        return NULL;
    }

    return file->inode;
}

/* Write inode */
static int ramfs_write_inode(inode_t *inode)
{
    ramfs_file_t *file;

    if (inode == NULL) {
        return -1;
    }

    file = (ramfs_file_t *)inode->fs_private;
    if (file == NULL) {
        return -1;
    }

    /* Update file metadata from inode */
    file->mode = inode->mode;
    file->size = inode->size;

    return 0;
}

/* Lookup file in ramfs */
static inode_t *ramfs_lookup(inode_t *dir, const char *name)
{
    ramfs_file_t *parent_file, *file;

    if (dir == NULL || name == NULL) {
        return NULL;
    }

    parent_file = (ramfs_file_t *)dir->fs_private;
    file = ramfs_find_file(parent_file, name);

    if (file == NULL) {
        return NULL;
    }

    return file->inode;
}

/* Read from ramfs file */
static ssize_t ramfs_read(file_t *file, void *buf, size_t count)
{
    ramfs_file_t *rf;
    size_t to_read;

    if (file == NULL || file->inode == NULL || buf == NULL) {
        return -1;
    }

    rf = (ramfs_file_t *)file->inode->fs_private;
    if (rf == NULL || rf->data == NULL) {
        return 0;
    }

    /* Check if position is beyond file size */
    if (file->pos >= rf->size) {
        return 0;
    }

    /* Calculate bytes to read */
    to_read = count;
    if (file->pos + to_read > rf->size) {
        to_read = rf->size - file->pos;
    }

    /* Copy data */
    memcpy(buf, (char *)rf->data + file->pos, to_read);
    file->pos += to_read;

    return (ssize_t)to_read;
}

/* Write to ramfs file */
static ssize_t ramfs_write(file_t *file, const void *buf, size_t count)
{
    ramfs_file_t *rf;
    u32 new_size;

    if (file == NULL || file->inode == NULL || buf == NULL) {
        return -1;
    }

    rf = (ramfs_file_t *)file->inode->fs_private;
    if (rf == NULL) {
        return -1;
    }

    /* Check if we need to expand the file */
    new_size = file->pos + count;
    if (new_size > RAMFS_MAX_FILE_SIZE) {
        return -1;  /* File too large */
    }

    if (new_size > rf->capacity) {
        /* Reallocate buffer */
        void *new_data = kmalloc(new_size);
        if (new_data == NULL) {
            return -1;
        }

        if (rf->data) {
            memcpy(new_data, rf->data, rf->size);
            kfree(rf->data);
        }

        rf->data = new_data;
        rf->capacity = new_size;
    }

    /* Copy data */
    memcpy((char *)rf->data + file->pos, buf, count);
    file->pos += count;

    /* Update size if we wrote past end of file */
    if (file->pos > rf->size) {
        rf->size = file->pos;
        file->inode->size = rf->size;
    }

    return (ssize_t)count;
}

/* Create directory */
static int ramfs_mkdir(inode_t *parent, const char *name, u32 mode)
{
    ramfs_file_t *parent_file, *file;
    inode_t *inode;

    if (ramfs_state.file_count >= RAMFS_MAX_FILES) {
        return -1;
    }

    parent_file = (ramfs_file_t *)parent->fs_private;

    /* Check if already exists */
    if (ramfs_find_file(parent_file, name) != NULL) {
        return -1;
    }

    file = &ramfs_state.files[ramfs_state.file_count];

    /* Copy name */
    const char *src = name;
    char *dst = file->name;
    int i = 0;
    while (*src && i < 63) {
        *dst++ = *src++;
        i++;
    }
    *dst = '\0';

    file->ino = ramfs_state.next_ino++;
    file->mode = S_IFDIR | mode;
    file->size = 0;
    file->data = NULL;
    file->capacity = 0;
    file->parent = parent_file;
    file->next = NULL;

    /* Create inode */
    inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (inode == NULL) {
        return -1;
    }

    inode->ino = file->ino;
    inode->mode = file->mode;
    inode->nlink = 2;
    inode->uid = 0;
    inode->gid = 0;
    inode->size = 0;
    inode->parent = parent;
    inode->next = NULL;
    inode->fs_private = (void *)file;

    file->inode = inode;

    ramfs_state.file_count++;
    parent->nlink++;

    return 0;
}

/* Read directory */
static int ramfs_readdir(inode_t *dir, dirent_t *entries, int count)
{
    ramfs_file_t *parent_file;
    int i, n = 0;

    if (dir == NULL || entries == NULL) {
        return 0;
    }

    parent_file = (ramfs_file_t *)dir->fs_private;

    /* Return all files in this directory */
    for (i = 0; i < ramfs_state.file_count && n < count; i++) {
        if (ramfs_state.files[i].parent == parent_file) {
            entries[n].ino = ramfs_state.files[i].ino;
            entries[n].type = (ramfs_state.files[i].mode & S_IFMT);

            /* Copy name */
            const char *src = ramfs_state.files[i].name;
            char *dst = entries[n].name;
            int j = 0;
            while (*src && j < 255) {
                *dst++ = *src++;
                j++;
            }
            *dst = '\0';

            entries[n].reclen = sizeof(dirent_t);
            n++;
        }
    }

    return n;
}

/* ramfs operations */
static fs_ops_t ramfs_ops = {
    .mount = ramfs_mount,
    .unmount = ramfs_unmount,
    .read_inode = ramfs_read_inode,
    .write_inode = ramfs_write_inode,
    .delete_inode = NULL,
    .open = NULL,
    .close = NULL,
    .read = ramfs_read,
    .write = ramfs_write,
    .seek = NULL,
    .mkdir = ramfs_mkdir,
    .rmdir = NULL,
    .readdir = ramfs_readdir,
    .lookup = ramfs_lookup,
    .name = "ramfs"
};

/* Initialize ramfs */
int ramfs_init(void)
{
    extern int vfs_register_fs(const char *name, fs_ops_t *ops);

    early_puts("ramfs: Initializing RAM filesystem\n");

    return vfs_register_fs("ramfs", &ramfs_ops);
}
