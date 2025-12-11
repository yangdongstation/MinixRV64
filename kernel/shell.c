/* Simple kernel shell implementation */

#include <minix/config.h>
#include <minix/shell.h>
#include <minix/board.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* External functions */
extern void early_puts(const char *s);
extern void early_putchar(char c);
extern void early_puthex(unsigned long val);
extern char uart_getchar(void);
extern int uart_haschar(void);

/* String functions */
extern int strcmp(const char *s1, const char *s2);
extern char *strncpy(char *dest, const char *src, unsigned long n);
extern unsigned long strlen(const char *s);

/* VFS types */
typedef struct file file_t;

/* VFS functions */
extern file_t *vfs_open(const char *path, int flags);
extern int vfs_close(file_t *file);
extern long vfs_read(file_t *file, void *buf, unsigned long count);
extern long vfs_write(file_t *file, const void *buf, unsigned long count);
extern int vfs_mkdir(const char *path, int mode);
extern int vfs_readdir(const char *path, void *dirents, int count);
extern int vfs_mount(const char *device, const char *mount_point, const char *fstype);

/* VFS dirent structure - must match vfs.h */
struct vfs_dirent {
    unsigned long ino;
    unsigned short reclen;
    unsigned char type;
    char name[256];
};

/* Command buffer */
static char cmd_buffer[SHELL_MAX_CMD_LEN];
static int cmd_pos = 0;

/* Forward declarations for command functions */
int cmd_help(int argc, char **argv);
int cmd_clear(int argc, char **argv);
int cmd_echo(int argc, char **argv);
int cmd_ls(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_pwd(int argc, char **argv);
int cmd_cd(int argc, char **argv);
int cmd_mkdir(int argc, char **argv);
int cmd_touch(int argc, char **argv);
int cmd_write(int argc, char **argv);
int cmd_rm(int argc, char **argv);
int cmd_mount(int argc, char **argv);
int cmd_ps(int argc, char **argv);
int cmd_kill(int argc, char **argv);
int cmd_reboot(int argc, char **argv);
int cmd_uname(int argc, char **argv);

/* Command table */
static struct shell_cmd commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear screen", cmd_clear},
    {"echo", "Echo arguments", cmd_echo},
    {"ls", "List directory contents", cmd_ls},
    {"cat", "Display file contents", cmd_cat},
    {"pwd", "Print working directory", cmd_pwd},
    {"cd", "Change directory", cmd_cd},
    {"mkdir", "Create directory", cmd_mkdir},
    {"touch", "Create empty file", cmd_touch},
    {"write", "Write text to file", cmd_write},
    {"rm", "Remove file", cmd_rm},
    {"mount", "Mount filesystem", cmd_mount},
    {"ps", "List processes", cmd_ps},
    {"kill", "Kill process", cmd_kill},
    {"reboot", "Reboot system", cmd_reboot},
    {"uname", "Show system information", cmd_uname},
    {NULL, NULL, NULL}
};

/* Initialize shell */
void shell_init(void)
{
    early_puts("✓ Shell\n");
}

/* Print shell prompt */
static void shell_prompt(void)
{
    early_puts("minix# ");
}

/* Parse command line into argc/argv */
static int shell_parse(char *cmdline, char **argv)
{
    int argc = 0;
    char *p = cmdline;

    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t') p++;

    while (*p && argc < SHELL_MAX_ARGS) {
        argv[argc++] = p;

        /* Find end of argument */
        while (*p && *p != ' ' && *p != '\t') p++;

        /* Null-terminate argument */
        if (*p) {
            *p++ = '\0';
            /* Skip whitespace */
            while (*p == ' ' || *p == '\t') p++;
        }
    }

    return argc;
}

/* Execute a command */
int shell_execute(const char *cmdline)
{
    char *argv[SHELL_MAX_ARGS];
    char cmd_copy[SHELL_MAX_CMD_LEN];
    int argc;
    int i;

    /* Copy command line */
    strncpy(cmd_copy, cmdline, SHELL_MAX_CMD_LEN - 1);
    cmd_copy[SHELL_MAX_CMD_LEN - 1] = '\0';

    /* Parse into argc/argv */
    argc = shell_parse(cmd_copy, argv);
    if (argc == 0) return 0;

    /* Find and execute command */
    for (i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            return commands[i].func(argc, argv);
        }
    }

    /* Command not found */
    early_puts("Unknown command: ");
    early_puts(argv[0]);
    early_puts("\n");
    return -1;
}

/* Main shell loop */
void shell_run(void)
{
    char c;
    volatile int delay;

    shell_prompt();

    while (1) {
        /* Wait for and get input character (non-blocking) */
        c = uart_getchar();

        /* If no character available, add delay and continue */
        if (c == '\0') {
            /* Add delay to reduce MMIO read frequency */
            for (delay = 0; delay < 100000; delay++)
                ;
            continue;
        }

        /* Handle special characters */
        if (c == '\r' || c == '\n') {
            /* End of command */
            early_putchar('\n');
            cmd_buffer[cmd_pos] = '\0';

            if (cmd_pos > 0) {
                shell_execute(cmd_buffer);
            }

            cmd_pos = 0;
            shell_prompt();
        } else if (c == 127 || c == 8) {
            /* Backspace */
            if (cmd_pos > 0) {
                cmd_pos--;
                early_puts("\b \b");
            }
        } else if (c >= 32 && c < 127) {
            /* Printable character */
            if (cmd_pos < SHELL_MAX_CMD_LEN - 1) {
                cmd_buffer[cmd_pos++] = c;
                early_putchar(c);
            }
        }
    }
}

/* Built-in commands */

int cmd_help(int argc, char **argv)
{
    int i;

    (void)argc;
    (void)argv;

    early_puts("Available commands:\n");
    for (i = 0; commands[i].name != NULL; i++) {
        early_puts("  ");
        early_puts(commands[i].name);
        early_puts(" - ");
        early_puts(commands[i].help);
        early_puts("\n");
    }

    return 0;
}

int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* ANSI escape sequence to clear screen */
    early_puts("\033[2J\033[H");
    return 0;
}

int cmd_echo(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (i > 1) early_putchar(' ');
        early_puts(argv[i]);
    }
    early_putchar('\n');

    return 0;
}

int cmd_ls(int argc, char **argv)
{
    struct vfs_dirent dirents[32];
    const char *path = "/";
    int count, i;

    /* Use provided path or default to "/" */
    if (argc >= 2) {
        path = argv[1];
    }

    early_puts("[ls] Reading directory: ");
    early_puts(path);
    early_puts("\n");

    /* Read directory */
    count = vfs_readdir(path, dirents, 32);

    early_puts("[ls] vfs_readdir returned count = ");
    early_puthex(count);
    early_puts("\n");

    if (count < 0) {
        early_puts("ls: cannot access '");
        early_puts(path);
        early_puts("'\n");
        return -1;
    }

    /* Print entries */
    if (count == 0) {
        early_puts("(empty directory)\n");
    } else {
        for (i = 0; i < count; i++) {
            early_puts(dirents[i].name);
            early_puts("\n");
        }
    }

    return 0;
}

int cmd_cat(int argc, char **argv)
{
    char buffer[256];
    file_t *file;
    long bytes_read;

    if (argc < 2) {
        early_puts("Usage: cat <file>\n");
        return -1;
    }

    /* Open file */
    file = vfs_open(argv[1], 0);  /* O_RDONLY = 0 */
    if (file == NULL) {
        early_puts("cat: cannot open '");
        early_puts(argv[1]);
        early_puts("'\n");
        return -1;
    }

    /* Read and print file contents */
    while ((bytes_read = vfs_read(file, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        early_puts(buffer);
    }

    vfs_close(file);
    return 0;
}

int cmd_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("/\n");
    return 0;
}

int cmd_cd(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("cd: filesystem not yet mounted\n");
    return 0;
}

int cmd_mkdir(int argc, char **argv)
{
    if (argc < 2) {
        early_puts("Usage: mkdir <directory>\n");
        return -1;
    }

    if (vfs_mkdir(argv[1], 0755) < 0) {
        early_puts("mkdir: cannot create directory '");
        early_puts(argv[1]);
        early_puts("'\n");
        return -1;
    }

    return 0;
}

int cmd_touch(int argc, char **argv)
{
    file_t *file;

    if (argc < 2) {
        early_puts("Usage: touch <file>\n");
        return -1;
    }

    /* Create file with O_CREAT | O_WRONLY = 0x101 */
    file = vfs_open(argv[1], 0x101);
    if (file == NULL) {
        early_puts("touch: cannot create '");
        early_puts(argv[1]);
        early_puts("'\n");
        return -1;
    }

    vfs_close(file);
    return 0;
}

int cmd_write(int argc, char **argv)
{
    char buffer[512];
    file_t *file;
    int i;
    unsigned long pos = 0;
    unsigned long len;

    if (argc < 3) {
        early_puts("Usage: write <file> <text...>\n");
        return -1;
    }

    /* Concatenate all arguments starting from argv[2] */
    for (i = 2; i < argc && pos < sizeof(buffer) - 2; i++) {
        const char *arg = argv[i];
        unsigned long j;

        /* Copy argument */
        for (j = 0; arg[j] != '\0' && pos < sizeof(buffer) - 2; j++) {
            buffer[pos++] = arg[j];
        }

        /* Add space between arguments */
        if (i < argc - 1) {
            buffer[pos++] = ' ';
        }
    }
    buffer[pos++] = '\n';
    buffer[pos] = '\0';

    /* Open file for writing (O_CREAT | O_WRONLY | O_TRUNC = 0x301) */
    file = vfs_open(argv[1], 0x301);
    if (file == NULL) {
        early_puts("write: cannot open '");
        early_puts(argv[1]);
        early_puts("'\n");
        return -1;
    }

    /* Write buffer to file */
    len = strlen(buffer);
    if (vfs_write(file, buffer, len) < 0) {
        early_puts("write: write failed\n");
        vfs_close(file);
        return -1;
    }

    vfs_close(file);
    return 0;
}

int cmd_mount(int argc, char **argv)
{
    if (argc < 4) {
        early_puts("Usage: mount <device> <mount_point> <fstype>\n");
        return -1;
    }

    if (vfs_mount(argv[1], argv[2], argv[3]) < 0) {
        early_puts("mount: mount failed\n");
        return -1;
    }

    early_puts("Mounted ");
    early_puts(argv[3]);
    early_puts(" on ");
    early_puts(argv[2]);
    early_puts("\n");
    return 0;
}

int cmd_rm(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("rm: not implemented yet\n");
    return 0;
}

int cmd_ps(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("PID  CMD\n");
    early_puts("  1  kernel\n");
    return 0;
}

int cmd_kill(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("kill: not implemented\n");
    return 0;
}

int cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("Rebooting...\n");
    /* TODO: implement actual reboot */
    while (1) {
        asm volatile ("wfi");
    }
    return 0;
}

int cmd_uname(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("Minix RV64 for RISC-V 64-bit\n");
    early_puts("Board: ");
    early_puts(BOARD_NAME);
    early_puts("\n");
    return 0;
}
