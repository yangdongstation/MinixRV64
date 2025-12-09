/* Virtual File System Interface */

#ifndef _MINIX_VFS_H
#define _MINIX_VFS_H

#include <types.h>

/* Inode structure */
typedef struct inode {
    u64 ino;                    /* Inode number */
    u32 mode;                   /* File mode (type and permissions) */
    u32 nlink;                  /* Number of hard links */
    u32 uid;                    /* User ID */
    u32 gid;                    /* Group ID */
    u64 size;                   /* File size */
    u64 atime;                  /* Access time */
    u64 mtime;                  /* Modify time */
    u64 ctime;                  /* Change time */
    u32 blksize;                /* Block size */
    u64 blocks;                 /* Number of blocks */
    void *fs_private;           /* Filesystem-specific data */
    struct inode *parent;       /* Parent directory */
    struct inode *next;         /* Hash chain */
} inode_t;

/* Directory entry */
typedef struct dirent {
    u64 ino;                    /* Inode number */
    u16 reclen;                 /* Record length */
    u8 type;                    /* File type */
    char name[256];             /* File name */
} dirent_t;

/* File structure */
typedef struct file {
    inode_t *inode;             /* Associated inode */
    u64 pos;                    /* Current position */
    u32 flags;                  /* Open flags */
    void *private;              /* Filesystem-specific data */
} file_t;

/* Filesystem operations */
typedef struct fs_ops {
    /* Filesystem operations */
    int (*mount)(const char *device, const char *mount_point);
    int (*unmount)(const char *mount_point);

    /* Inode operations */
    inode_t *(*read_inode)(u64 ino);
    int (*write_inode)(inode_t *inode);
    int (*delete_inode)(inode_t *inode);

    /* File operations */
    int (*open)(inode_t *inode, file_t *file, int flags);
    int (*close)(file_t *file);
    ssize_t (*read)(file_t *file, void *buf, size_t count);
    ssize_t (*write)(file_t *file, const void *buf, size_t count);
    int (*seek)(file_t *file, s64 offset, int whence);

    /* Directory operations */
    int (*mkdir)(inode_t *parent, const char *name, u32 mode);
    int (*rmdir)(inode_t *parent, const char *name);
    int (*readdir)(inode_t *dir, dirent_t *entries, int count);
    inode_t *(*lookup)(inode_t *dir, const char *name);

    /* Name: filesystem identifier */
    const char *name;
} fs_ops_t;

/* File types */
#define S_IFMT      0xF000      /* Mask for file type */
#define S_IFREG     0x8000      /* Regular file */
#define S_IFDIR     0x4000      /* Directory */
#define S_IFCHR     0x2000      /* Character device */
#define S_IFBLK     0x6000      /* Block device */
#define S_IFIFO     0x1000      /* FIFO */
#define S_IFLNK     0xA000      /* Symbolic link */

/* File permissions */
#define S_IRUSR     0x100       /* User read */
#define S_IWUSR     0x80        /* User write */
#define S_IXUSR     0x40        /* User execute */
#define S_IRGRP     0x20        /* Group read */
#define S_IWGRP     0x10        /* Group write */
#define S_IXGRP     0x8         /* Group execute */
#define S_IROTH     0x4         /* Other read */
#define S_IWOTH     0x2         /* Other write */
#define S_IXOTH     0x1         /* Other execute */

/* Open flags */
#define O_RDONLY    0x0         /* Read-only */
#define O_WRONLY    0x1         /* Write-only */
#define O_RDWR      0x2         /* Read-write */
#define O_APPEND    0x8         /* Append mode */
#define O_CREAT     0x100       /* Create if not exists */
#define O_TRUNC     0x200       /* Truncate */

/* Seek whence */
#define SEEK_SET    0           /* Beginning of file */
#define SEEK_CUR    1           /* Current position */
#define SEEK_END    2           /* End of file */

/* VFS functions */
int vfs_init(void);
int vfs_register_fs(const char *name, fs_ops_t *ops);
int vfs_mount(const char *device, const char *mount_point, const char *fstype);
int vfs_unmount(const char *mount_point);
inode_t *vfs_lookup_path(const char *path);
file_t *vfs_open(const char *path, int flags);
int vfs_close(file_t *file);
ssize_t vfs_read(file_t *file, void *buf, size_t count);
ssize_t vfs_write(file_t *file, const void *buf, size_t count);
int vfs_mkdir(const char *path, u32 mode);
int vfs_rmdir(const char *path);

#endif /* _MINIX_VFS_H */
