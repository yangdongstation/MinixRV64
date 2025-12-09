/* Minix RV64 system calls */

#include <minix/config.h>
#include <types.h>
#include <minix/process.h>
#include <minix/vfs.h>

/* Define NULL and error codes */
#ifndef NULL
#define NULL ((void *)0)
#endif

#define EBADF   (-1)    /* Bad file descriptor */
#define EINVAL  (-2)    /* Invalid argument */
#define EIO     (-3)    /* I/O error */

/* System call numbers */
#define SYS_fork        1
#define SYS_exit        2
#define SYS_wait        3
#define SYS_read        4
#define SYS_write       5
#define SYS_open        6
#define SYS_close       7
#define SYS_exec        8
#define SYS_getpid      9
#define SYS_kill        10

/* Forward declarations */
extern struct proc *get_current_proc(void);
extern void early_putchar(char c);
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);
extern pid_t fork(void);
extern int exec(const char *path, char **argv);
extern void exit(int status);
extern int wait(int *status);

/* File descriptor allocation */
static int fdalloc(struct proc *p, file_desc_t *f)
{
    int fd;
    for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (p->ofile[fd] == NULL) {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

/* System call implementations */

/* sys_read: Read from file descriptor */
ssize_t sys_read(int fd, void *buf, size_t count)
{
    struct proc *p = get_current_proc();
    file_desc_t *f;
    file_t *vfs_file;

    if (p == NULL || buf == NULL) {
        return EINVAL;
    }

    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return EBADF;
    }

    f = p->ofile[fd];
    if (f == NULL || !f->readable) {
        return EBADF;
    }

    vfs_file = (file_t *)f->data;
    if (vfs_file == NULL) {
        return EBADF;
    }

    return vfs_read(vfs_file, buf, count);
}

/* sys_write: Write to file descriptor */
ssize_t sys_write(int fd, const void *buf, size_t count)
{
    struct proc *p = get_current_proc();
    file_desc_t *f;
    file_t *vfs_file;
    const char *cbuf;
    size_t i;

    if (p == NULL || buf == NULL) {
        return EINVAL;
    }

    /* Special case: fd 1 (stdout) and fd 2 (stderr) go to console */
    if (fd == 1 || fd == 2) {
        cbuf = (const char *)buf;
        for (i = 0; i < count; i++) {
            early_putchar(cbuf[i]);
        }
        return (ssize_t)count;
    }

    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return EBADF;
    }

    f = p->ofile[fd];
    if (f == NULL || !f->writable) {
        return EBADF;
    }

    vfs_file = (file_t *)f->data;
    if (vfs_file == NULL) {
        return EBADF;
    }

    return vfs_write(vfs_file, buf, count);
}

/* sys_open: Open file */
int sys_open(const char *path, int flags)
{
    struct proc *p = get_current_proc();
    file_desc_t *f;
    file_t *vfs_file;
    int fd;

    if (p == NULL || path == NULL) {
        return EINVAL;
    }

    /* Open file through VFS */
    vfs_file = vfs_open(path, flags);
    if (vfs_file == NULL) {
        return EIO;
    }

    /* Allocate file descriptor structure */
    f = (file_desc_t *)kmalloc(sizeof(file_desc_t));
    if (f == NULL) {
        vfs_close(vfs_file);
        return EIO;
    }

    /* Initialize file descriptor */
    f->type = S_IFREG;
    f->ref = 1;
    f->readable = (flags & O_WRONLY) ? 0 : 1;
    f->writable = (flags & O_RDONLY) ? 0 : 1;
    f->data = vfs_file;
    f->off = 0;

    /* Allocate file descriptor number */
    fd = fdalloc(p, f);
    if (fd < 0) {
        vfs_close(vfs_file);
        kfree(f);
        return EBADF;
    }

    return fd;
}

/* sys_close: Close file descriptor */
int sys_close(int fd)
{
    struct proc *p = get_current_proc();
    file_desc_t *f;
    file_t *vfs_file;

    if (p == NULL) {
        return EINVAL;
    }

    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return EBADF;
    }

    f = p->ofile[fd];
    if (f == NULL) {
        return EBADF;
    }

    /* Close VFS file */
    vfs_file = (file_t *)f->data;
    if (vfs_file != NULL) {
        vfs_close(vfs_file);
    }

    /* Free file structure */
    kfree(f);

    /* Clear file descriptor */
    p->ofile[fd] = NULL;

    return 0;
}

/* System call handler */
void syscall_handler(unsigned long a0, unsigned long a1,
                    unsigned long a2, unsigned long a3,
                    unsigned long a4, unsigned long a5,
                    unsigned long syscall_num)
{
    /* Suppress unused parameter warnings for now */
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;

    switch (syscall_num) {
    case SYS_fork:
        a0 = (unsigned long)fork();
        break;

    case SYS_exit:
        exit((int)a0);
        /* exit() does not return */
        break;

    case SYS_wait:
        a0 = (unsigned long)wait((int *)a0);
        break;

    case SYS_exec:
        a0 = (unsigned long)exec((const char *)a0, (char **)a1);
        break;

    case SYS_read:
        a0 = (unsigned long)sys_read((int)a0, (void *)a1, (size_t)a2);
        break;

    case SYS_write:
        a0 = (unsigned long)sys_write((int)a0, (const void *)a1, (size_t)a2);
        break;

    case SYS_open:
        a0 = (unsigned long)sys_open((const char *)a0, (int)a1);
        break;

    case SYS_close:
        a0 = (unsigned long)sys_close((int)a0);
        break;

    case SYS_getpid:
        {
            struct proc *p = get_current_proc();
            a0 = (p != NULL) ? (unsigned long)p->pid : (unsigned long)-1;
        }
        break;

    default:
        /* Unknown system call */
        a0 = (unsigned long)-1;
        break;
    }
}