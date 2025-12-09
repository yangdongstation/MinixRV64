/* Kernel print functions */

#ifndef _MINIX_PRINT_H
#define _MINIX_PRINT_H

#include <types.h>

/* Print levels */
#define KERN_EMERG   "<0>"
#define KERN_ALERT   "<1>"
#define KERN_CRIT    "<2>"
#define KERN_ERR     "<3>"
#define KERN_WARNING "<4>"
#define KERN_NOTICE  "<5>"
#define KERN_INFO    "<6>"
#define KERN_DEBUG   "<7>"

/* Print functions */
void printk(const char *fmt, ...);
void printf(const char *fmt, ...);
void puts(const char *s);

/* Simple debug print */
#define pr_debug(fmt, ...) \
    printk(KERN_DEBUG fmt, ##__VA_ARGS__)

#define pr_info(fmt, ...) \
    printk(KERN_INFO fmt, ##__VA_ARGS__)

#define pr_err(fmt, ...) \
    printk(KERN_ERR fmt, ##__VA_ARGS__)

#endif /* _MINIX_PRINT_H */