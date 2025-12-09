/* String Library Implementation */

#include <types.h>

/**
 * memcpy - Copy memory area
 * @dest: Destination pointer
 * @src: Source pointer
 * @n: Number of bytes to copy
 * @return: Destination pointer
 */
void *memcpy(void *dest, const void *src, unsigned long n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    while (n--) {
        *d++ = *s++;
    }

    return dest;
}

/**
 * memset - Fill memory with a constant byte
 * @s: Memory area pointer
 * @c: Byte value
 * @n: Number of bytes to fill
 * @return: Memory area pointer
 */
void *memset(void *s, int c, unsigned long n)
{
    unsigned char *p = (unsigned char *)s;
    unsigned char val = (unsigned char)c;

    while (n--) {
        *p++ = val;
    }

    return s;
}

/**
 * memcmp - Compare memory areas
 * @s1: First memory area
 * @s2: Second memory area
 * @n: Number of bytes to compare
 * @return: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int memcmp(const void *s1, const void *s2, unsigned long n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }

    return 0;
}

/**
 * strlen - Calculate string length
 * @s: String pointer
 * @return: Length of string
 */
unsigned long strlen(const char *s)
{
    const char *p = s;

    while (*p) {
        p++;
    }

    return p - s;
}

/**
 * strcmp - Compare two strings
 * @s1: First string
 * @s2: Second string
 * @return: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/**
 * strncmp - Compare two strings up to n characters
 * @s1: First string
 * @s2: Second string
 * @n: Maximum number of characters to compare
 * @return: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int strncmp(const char *s1, const char *s2, unsigned long n)
{
    if (n == 0) {
        return 0;
    }

    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    if (n == (unsigned long)-1) {
        return 0;
    }

    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/**
 * strcpy - Copy a string
 * @dest: Destination buffer
 * @src: Source string
 * @return: Destination pointer
 */
char *strcpy(char *dest, const char *src)
{
    char *ret = dest;

    while ((*dest++ = *src++))
        ;

    return ret;
}

/**
 * strncpy - Copy a string up to n characters
 * @dest: Destination buffer
 * @src: Source string
 * @n: Maximum number of characters to copy
 * @return: Destination pointer
 */
char *strncpy(char *dest, const char *src, unsigned long n)
{
    char *ret = dest;

    while (n && (*dest++ = *src++)) {
        n--;
    }

    while (n--) {
        *dest++ = '\0';
    }

    return ret;
}
