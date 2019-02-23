//
// Created by rebut_p on 17/12/18.
//

#include <kernel/string.h>

int memcmp(const void *s1, const void *s2, u32 n)
{
    const char *d1 = s1;
    const char *d2 = s2;

    for (u32 i = 0; i < n; ++i)
        if (d1[i] != d2[i])
            return d1[i] - d2[i];

    return 0;
}

void *memcpy(void *dest, const void *src, u32 n)
{
    const char *s = src;
    char *d = dest;

    for (u32 i = 0; i < n; i++)
        *d++ = *s++;

    return dest;
}

void *memset(void *s, int c, u32 n)
{
    char *p = s;

    for (u32 i = 0; i < n; ++i)
        p[i] = c;

    return s;
}

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    const unsigned char *end = p + n;

    for (;;) {
        if (p >= end || p[0] == c)
            break;
        p++;
        if (p >= end || p[0] == c)
            break;
        p++;
        if (p >= end || p[0] == c)
            break;
        p++;
        if (p >= end || p[0] == c)
            break;
        p++;
    }

    if (p >= end)
        return NULL;
    return (void *) p;
}