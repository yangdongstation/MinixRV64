/* System call definitions */

#ifndef _MINIX_SYSCALL_H
#define _MINIX_SYSCALL_H

/* System call numbers */
#define SYS_NONE          0
#define SYS_fork          1
#define SYS_exit          2
#define SYS_wait          3
#define SYS_read          4
#define SYS_write         5
#define SYS_open          6
#define SYS_close         7
#define SYS_execve        8
#define SYS_getpid        9
#define SYS_kill          10
#define SYS_lseek         11
#define SYS_brk           12
#define SYS_gettimeofday  13
#define SYS_mmap          14
#define SYS_munmap        15
#define SYS_socket        16
#define SYS_connect       17
#define SYS_accept        18

/* Minix-specific system calls */
#define SYS_sendrec       20
#define SYS_send          21
#define SYS_receive       22
#define SYS_notify        23
#define SYS_getksig       24
#define SYS_endksig       25
#define SYS_copy          26
#define SYS_vcopy         27
#define SYS_umap          28
#define SYS_trace         29

/* System call return values */
#define SYS_SUCCESS       0
#define SYS_ERROR        (-1)

/* Maximum system call number */
#define MAX_SYSCALL       64

#endif /* _MINIX_SYSCALL_H */