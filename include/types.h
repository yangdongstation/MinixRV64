/* Basic type definitions */

#ifndef _TYPES_H
#define _TYPES_H

/* Signed types */
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

/* Unsigned types */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

/* Size types */
typedef s64 ssize_t;
typedef u64 size_t;

/* Address types */
typedef u64 phys_addr_t;
typedef u64 virt_addr_t;

/* Boolean type */
typedef int bool;
#define true 1
#define false 0

/* Kernel types */
typedef unsigned long long ktime_t;

/* Process types */
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;

/* Resource types */
typedef unsigned int resource_t;

/* Status codes */
typedef int status_t;
#define OK      0
#define ERROR  -1

#endif /* _TYPES_H */