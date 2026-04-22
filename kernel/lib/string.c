#include "string.h"
#include <stdint.h>

size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}

char *strcpy(char *dest, const char *src)
{
    int i = 0;
    while (src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i = 0;
    while (i < n && src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    while (i < n)
    {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
    {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n > 0 && *a && (*a == *b))
    {
        a++;
        b++;
        n--;
    }
    if (n == 0)
    {
        return 0;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcat(char *dest, const char *src)
{
    char *end = dest + strlen(dest);
    int i = 0;
    while (src[i] != '\0')
    {
        end[i] = src[i];
        i++;
    }
    end[i] = '\0';
    return dest;
}

char *strchr(const char *str, int c)
{
    while (*str != '\0')
    {
        if (*str == (char)c)
        {
            return (char *)str;
        }
        str++;
    }
    if (c == '\0')
    {
        return (char *)str;
    }
    return 0;
}

void *memset(void *dest, int val, size_t n)
{
    uint8_t *ptr = (uint8_t *)dest;
    while (n > 0)
    {
        *ptr = (uint8_t)val;
        ptr++;
        n--;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n > 0)
    {
        *d = *s;
        d++;
        s++;
        n--;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d < s)
    {
        while (n > 0)
        {
            *d = *s;
            d++;
            s++;
            n--;
        }
    }
    else
    {
        d += n;
        s += n;
        while (n > 0)
        {
            d--;
            s--;
            *d = *s;
            n--;
        }
    }
    return dest;
}