/**
 * @file string.c
 * @brief Minimal string.h function implementations for DMOD modules
 * 
 * This file provides basic string manipulation functions to avoid
 * GCC's built-in function replacements that cause linking issues
 * in modules built with -nostdlib.
 */

#include <stddef.h>

/**
 * @brief Set memory to a specified value
 */
void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    unsigned char value = (unsigned char)c;
    
    while (n--)
    {
        *p++ = value;
    }
    
    return s;
}

/**
 * @brief Copy memory area
 * @note This function does not handle overlapping memory regions. Use memmove() for that.
 */
void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    
    while (n--)
    {
        *d++ = *s++;
    }
    
    return dest;
}

/**
 * @brief Move memory area (handles overlapping regions)
 */
void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    
    if (d < s)
    {
        /* Copy forward */
        while (n--)
        {
            *d++ = *s++;
        }
    }
    else if (d > s)
    {
        /* Copy backward */
        d += n;
        s += n;
        while (n--)
        {
            *--d = *--s;
        }
    }
    
    return dest;
}

/**
 * @brief Calculate the length of a string
 */
size_t strlen(const char *s)
{
    size_t len = 0;
    
    while (*s++)
    {
        len++;
    }
    
    return len;
}

/**
 * @brief Copy a string
 */
char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    
    while ((*dest++ = *src++))
    {
        /* Copy until null terminator */
    }
    
    return ret;
}

/**
 * @brief Copy at most n bytes of a string
 */
char *strncpy(char *dest, const char *src, size_t n)
{
    char *ret = dest;
    
    while (n > 0 && *src)
    {
        *dest++ = *src++;
        n--;
    }
    
    /* Pad with zeros if necessary */
    while (n > 0)
    {
        *dest++ = '\0';
        n--;
    }
    
    return ret;
}

/**
 * @brief Compare two strings
 */
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/**
 * @brief Compare at most n bytes of two strings
 */
int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
    {
        return 0;
    }
    
    while (n > 1 && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/**
 * @brief Concatenate two strings
 */
char *strcat(char *dest, const char *src)
{
    char *ret = dest;
    
    /* Find end of dest */
    while (*dest)
    {
        dest++;
    }
    
    /* Copy src */
    while ((*dest++ = *src++))
    {
        /* Copy until null terminator */
    }
    
    return ret;
}

/**
 * @brief Concatenate at most n bytes from src to dest
 */
char *strncat(char *dest, const char *src, size_t n)
{
    char *ret = dest;
    
    /* Find end of dest */
    while (*dest)
    {
        dest++;
    }
    
    /* Copy at most n bytes from src */
    while (n > 0 && *src)
    {
        *dest++ = *src++;
        n--;
    }
    
    /* Null terminate */
    *dest = '\0';
    
    return ret;
}

/**
 * @brief Locate character in string
 */
char *strchr(const char *s, int c)
{
    while (*s)
    {
        if (*s == (char)c)
        {
            return (char *)s;
        }
        s++;
    }
    
    /* Check for null terminator match */
    if ((char)c == '\0')
    {
        return (char *)s;
    }
    
    return NULL;
}

/**
 * @brief Locate last occurrence of character in string
 */
char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    
    while (*s)
    {
        if (*s == (char)c)
        {
            last = s;
        }
        s++;
    }
    
    /* Check for null terminator match */
    if ((char)c == '\0')
    {
        return (char *)s;
    }
    
    return (char *)last;
}
