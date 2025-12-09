/* Simple shell interface */

#ifndef _MINIX_SHELL_H
#define _MINIX_SHELL_H

/* Maximum command line length */
#define SHELL_MAX_CMD_LEN 256

/* Maximum number of arguments */
#define SHELL_MAX_ARGS 16

/* Shell command structure */
struct shell_cmd {
    const char *name;
    const char *help;
    int (*func)(int argc, char **argv);
};

/* Shell functions */
void shell_init(void);
void shell_run(void);
int shell_execute(const char *cmdline);

/* Built-in commands */
int cmd_help(int argc, char **argv);
int cmd_clear(int argc, char **argv);
int cmd_echo(int argc, char **argv);
int cmd_ls(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_pwd(int argc, char **argv);
int cmd_cd(int argc, char **argv);
int cmd_mkdir(int argc, char **argv);
int cmd_rm(int argc, char **argv);
int cmd_ps(int argc, char **argv);
int cmd_kill(int argc, char **argv);
int cmd_reboot(int argc, char **argv);
int cmd_uname(int argc, char **argv);

#endif /* _MINIX_SHELL_H */
