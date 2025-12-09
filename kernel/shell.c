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
extern char uart_getchar(void);
extern int uart_haschar(void);

/* String functions */
extern int strcmp(const char *s1, const char *s2);
extern char *strncpy(char *dest, const char *src, unsigned long n);

/* Command buffer */
static char cmd_buffer[SHELL_MAX_CMD_LEN];
static int cmd_pos = 0;

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
    {"rm", "Remove file", cmd_rm},
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
    (void)argc;
    (void)argv;

    early_puts("ls: filesystem not yet mounted\n");
    return 0;
}

int cmd_cat(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("cat: filesystem not yet mounted\n");
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
    (void)argc;
    (void)argv;

    early_puts("mkdir: filesystem not yet mounted\n");
    return 0;
}

int cmd_rm(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    early_puts("rm: filesystem not yet mounted\n");
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
