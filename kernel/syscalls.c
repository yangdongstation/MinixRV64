/* MinixRV64 Donz Build - System Calls
 *
 * System call interface using new process management
 * Following HowToFitPosix.md Stage 2 design
 */

#include <minix/config.h>
#include <minix/task.h>
#include <minix/vfs.h>
#include <types.h>

/* Define NULL and error codes */
#ifndef NULL
#define NULL ((void *)0)
#endif

#define EBADF   (-9)    /* Bad file descriptor */
#define EINVAL  (-22)   /* Invalid argument */
#define EIO     (-5)    /* I/O error */
#define ENOSYS  (-38)   /* Function not implemented */

/* System call numbers (Linux RISC-V convention) */
#define SYS_read        63
#define SYS_write       64
#define SYS_openat      56
#define SYS_close       57
#define SYS_fstat       80
#define SYS_lseek       62
#define SYS_exit        93
#define SYS_exit_group  94
#define SYS_getpid      172
#define SYS_getppid     173
#define SYS_clone       220
#define SYS_wait4       260
#define SYS_kill        129
#define SYS_brk         214
#define SYS_mmap        222
#define SYS_munmap      215
#define SYS_execve      221

/* Legacy syscall numbers for compatibility */
#define SYS_fork_legacy     1
#define SYS_exit_legacy     2
#define SYS_wait_legacy     3
#define SYS_exec_legacy     8

/* Forward declarations - new process API */
extern struct task_struct *get_current(void);
extern void early_putchar(char c);
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);

/* From fork.c */
extern pid_t sys_fork(void);
extern pid_t sys_clone(unsigned long clone_flags, unsigned long newsp,
                       void *parent_tidptr, unsigned long tls, void *child_tidptr);

/* From exit.c */
extern long sys_exit(int error_code);
extern long sys_exit_group(int error_code);
extern long sys_wait4(pid_t pid, int *stat_addr, int options, void *rusage);
extern long sys_kill(pid_t pid, int sig);

/* From exec.c */
extern long sys_execve(const char *filename, char **argv, char **envp);

/* File descriptor allocation */
static int fdalloc(struct task_struct *p, file_desc_t *f)
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

/* ============================================
 * System call implementations
 * ============================================ */

/* sys_read: Read from file descriptor */
ssize_t sys_read(int fd, void *buf, size_t count)
{
    struct task_struct *p = get_current();
    file_desc_t *f;
    file_t *vfs_file;

    if (p == NULL || buf == NULL) {
        return EINVAL;
    }

    /* Handle stdin (fd 0) - read from UART */
    if (fd == 0) {
        extern int uart_getchar(void);
        char *cbuf = (char *)buf;
        size_t i;
        for (i = 0; i < count; i++) {
            int c = uart_getchar();
            if (c < 0) break;
            cbuf[i] = (char)c;
            if (c == '\n') {
                i++;
                break;
            }
        }
        return (ssize_t)i;
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
    struct task_struct *p = get_current();
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

/* sys_open: Open file (openat with AT_FDCWD) */
int sys_open(const char *path, int flags)
{
    struct task_struct *p = get_current();
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
    struct task_struct *p = get_current();
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

/* sys_getpid: Get process ID */
pid_t sys_getpid(void)
{
    struct task_struct *p = get_current();
    return (p != NULL) ? p->pid : -1;
}

/* sys_getppid: Get parent process ID */
pid_t sys_getppid(void)
{
    struct task_struct *p = get_current();
    return (p != NULL) ? p->ppid : -1;
}

/* sys_brk: Change data segment size (simplified) */
long sys_brk(unsigned long brk)
{
    struct task_struct *p = get_current();
    if (p == NULL || p->mm == NULL) {
        return -1;
    }

    if (brk == 0) {
        return (long)p->mm->brk;
    }

    /* Simple implementation - just update brk pointer */
    if (brk >= p->mm->start_brk) {
        p->mm->brk = brk;
    }

    return (long)p->mm->brk;
}

/* ============================================
 * System call dispatcher
 * ============================================ */

long syscall_handler(unsigned long a0, unsigned long a1,
                     unsigned long a2, unsigned long a3,
                     unsigned long a4, unsigned long a5,
                     unsigned long syscall_num)
{
    long ret = -ENOSYS;

    /* Suppress unused parameter warnings */
    (void)a3;
    (void)a4;
    (void)a5;

    switch (syscall_num) {
    /* Process management */
    case SYS_clone:
        ret = sys_clone(a0, a1, (void *)a2, a3, (void *)a4);
        break;

    case SYS_fork_legacy:
        ret = sys_fork();
        break;

    case SYS_exit:
    case SYS_exit_legacy:
        ret = sys_exit((int)a0);
        break;

    case SYS_exit_group:
        ret = sys_exit_group((int)a0);
        break;

    case SYS_wait4:
    case SYS_wait_legacy:
        ret = sys_wait4((pid_t)a0, (int *)a1, (int)a2, (void *)a3);
        break;

    case SYS_execve:
    case SYS_exec_legacy:
        ret = sys_execve((const char *)a0, (char **)a1, (char **)a2);
        break;

    case SYS_kill:
        ret = sys_kill((pid_t)a0, (int)a1);
        break;

    case SYS_getpid:
        ret = sys_getpid();
        break;

    case SYS_getppid:
        ret = sys_getppid();
        break;

    /* File operations */
    case SYS_read:
        ret = sys_read((int)a0, (void *)a1, (size_t)a2);
        break;

    case SYS_write:
        ret = sys_write((int)a0, (const void *)a1, (size_t)a2);
        break;

    case SYS_openat:
        /* openat(AT_FDCWD, path, flags, mode) -> open(path, flags) */
        ret = sys_open((const char *)a1, (int)a2);
        break;

    case SYS_close:
        ret = sys_close((int)a0);
        break;

    /* Memory management */
    case SYS_brk:
        ret = sys_brk(a0);
        break;

    case SYS_mmap:
    case SYS_munmap:
        /* Not implemented yet */
        ret = -ENOSYS;
        break;

    default:
        /* Unknown system call */
        ret = -ENOSYS;
        break;
    }

    return ret;
}
