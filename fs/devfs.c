/* Device Filesystem (devfs) Implementation */

#include <minix/config.h>
#include <types.h>
#include <minix/vfs.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Maximum number of device nodes */
#define MAX_DEV_NODES 64

/* Device node types */
#define DEVFS_TYPE_CHAR  1
#define DEVFS_TYPE_BLOCK 2

/* Device node structure */
typedef struct dev_node {
    char name[64];
    u8 type;                /* DEVFS_TYPE_CHAR or DEVFS_TYPE_BLOCK */
    u32 major;              /* Major device number */
    u32 minor;              /* Minor device number */
    u32 mode;               /* Permissions */
    inode_t *inode;         /* Associated inode */
    struct dev_node *next;
} dev_node_t;

/* devfs state */
static struct {
    dev_node_t nodes[MAX_DEV_NODES];
    int node_count;
    inode_t *root_inode;
} devfs_state;

/* Forward declarations */
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);

/* Find device node by name */
static dev_node_t *devfs_find_node(const char *name)
{
    int i;
    const char *a, *b;

    for (i = 0; i < devfs_state.node_count; i++) {
        a = devfs_state.nodes[i].name;
        b = name;

        /* Compare strings */
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }

        if (*a == '\0' && *b == '\0') {
            return &devfs_state.nodes[i];
        }
    }

    return NULL;
}

/* Mount devfs */
static int devfs_mount(const char *device, const char *mount_point)
{
    (void)device;

    early_puts("devfs: Mounting on ");
    early_puts(mount_point);
    early_puts("\n");

    /* Initialize state */
    devfs_state.node_count = 0;

    early_puts("devfs: Allocating root inode...\n");

    /* Create root inode */
    devfs_state.root_inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (devfs_state.root_inode == NULL) {
        early_puts("devfs: Failed to allocate root inode\n");
        return -1;
    }

    early_puts("devfs: Initializing root inode...\n");

    devfs_state.root_inode->ino = 1;
    devfs_state.root_inode->mode = S_IFDIR | 0755;
    devfs_state.root_inode->nlink = 2;
    devfs_state.root_inode->uid = 0;
    devfs_state.root_inode->gid = 0;
    devfs_state.root_inode->size = 0;
    devfs_state.root_inode->parent = NULL;
    devfs_state.root_inode->next = NULL;
    devfs_state.root_inode->fs_private = NULL;

    early_puts("devfs: Mounted successfully\n");
    return 0;
}

/* Unmount devfs */
static int devfs_unmount(const char *mount_point)
{
    (void)mount_point;

    if (devfs_state.root_inode) {
        kfree(devfs_state.root_inode);
        devfs_state.root_inode = NULL;
    }

    return 0;
}

/* Read inode */
static inode_t *devfs_read_inode(u64 ino)
{
    if (ino == 1) {
        return devfs_state.root_inode;
    }

    /* Find inode in device nodes */
    for (int i = 0; i < devfs_state.node_count; i++) {
        if (devfs_state.nodes[i].inode &&
            devfs_state.nodes[i].inode->ino == ino) {
            return devfs_state.nodes[i].inode;
        }
    }

    return NULL;
}

/* Lookup file in devfs */
static inode_t *devfs_lookup(inode_t *dir, const char *name)
{
    dev_node_t *node;

    (void)dir;

    node = devfs_find_node(name);
    if (node == NULL) {
        return NULL;
    }

    return node->inode;
}

/* Read directory */
static int devfs_readdir(inode_t *dir, dirent_t *entries, int count)
{
    int i, n = 0;

    (void)dir;

    /* Return all device nodes */
    for (i = 0; i < devfs_state.node_count && n < count; i++) {
        entries[n].ino = devfs_state.nodes[i].inode->ino;
        entries[n].type = (devfs_state.nodes[i].type == DEVFS_TYPE_CHAR) ?
                          S_IFCHR : S_IFBLK;

        /* Copy name */
        const char *src = devfs_state.nodes[i].name;
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

    return n;
}

/* devfs operations */
static fs_ops_t devfs_ops = {
    .mount = devfs_mount,
    .unmount = devfs_unmount,
    .read_inode = devfs_read_inode,
    .write_inode = NULL,
    .delete_inode = NULL,
    .open = NULL,
    .close = NULL,
    .read = NULL,
    .write = NULL,
    .seek = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .readdir = devfs_readdir,
    .lookup = devfs_lookup,
    .name = "devfs"
};

/* Register device node */
int devfs_register_device(const char *name, u8 type, u32 major, u32 minor, u32 mode)
{
    dev_node_t *node;
    inode_t *inode;

    if (devfs_state.node_count >= MAX_DEV_NODES) {
        early_puts("devfs: Too many device nodes\n");
        return -1;
    }

    /* Check if already exists */
    if (devfs_find_node(name) != NULL) {
        early_puts("devfs: Device already exists: ");
        early_puts(name);
        early_puts("\n");
        return -1;
    }

    node = &devfs_state.nodes[devfs_state.node_count];

    /* Copy name */
    const char *src = name;
    char *dst = node->name;
    int i = 0;
    while (*src && i < 63) {
        *dst++ = *src++;
        i++;
    }
    *dst = '\0';

    node->type = type;
    node->major = major;
    node->minor = minor;
    node->mode = mode;

    /* Create inode */
    inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (inode == NULL) {
        return -1;
    }

    inode->ino = devfs_state.node_count + 2;  /* Start from 2 (1 is root) */
    inode->mode = (type == DEVFS_TYPE_CHAR ? S_IFCHR : S_IFBLK) | mode;
    inode->nlink = 1;
    inode->uid = 0;
    inode->gid = 0;
    inode->size = 0;
    inode->parent = devfs_state.root_inode;
    inode->next = NULL;
    inode->fs_private = (void *)node;

    node->inode = inode;
    node->next = NULL;

    devfs_state.node_count++;

    early_puts("devfs: Registered device: ");
    early_puts(name);
    early_puts("\n");

    return 0;
}

/* Unregister device node */
int devfs_unregister_device(const char *name)
{
    dev_node_t *node;
    int i, idx = -1;

    node = devfs_find_node(name);
    if (node == NULL) {
        return -1;
    }

    /* Find node index */
    for (i = 0; i < devfs_state.node_count; i++) {
        if (&devfs_state.nodes[i] == node) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        return -1;
    }

    /* Free inode */
    if (node->inode) {
        kfree(node->inode);
    }

    /* Shift remaining nodes */
    for (i = idx; i < devfs_state.node_count - 1; i++) {
        devfs_state.nodes[i] = devfs_state.nodes[i + 1];
    }

    devfs_state.node_count--;

    return 0;
}

/* Initialize devfs */
int devfs_init(void)
{
    extern int vfs_register_fs(const char *name, fs_ops_t *ops);

    early_puts("devfs: Initializing device filesystem\n");

    return vfs_register_fs("devfs", &devfs_ops);
}
