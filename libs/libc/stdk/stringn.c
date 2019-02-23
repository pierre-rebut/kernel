//
// Created by rebut_p on 17/12/18.
//

#include <kernel/string.h>

int strncmp(const char *s1, const char *s2, u32 n)
{
    for (u32 i = 0; i < n; ++i)
        if (s1[i] == '\0' || s1[i] != s2[i])
            return s1[i] - s2[i];

    return 0;
}

char *strncpy(char *dst, char const *src, u32 n)
{
    if (n != 0) {
        register char *d = dst;
        register const char *s = src;

        do {
            if ((*d++ = *s++) == 0) {
                while (--n != 0)
                    *d++ = 0;
                break;
            }
        } while (--n != 0);
    }
    return (dst);
}

u32 strnlen(const char *s, u32 maxlen)
{
    register u32 i = 0;
    for (; i < maxlen; ++i)
        if (!s[i])
            return i;
    return i;
}

int strncontain(const char *s, char c, u32 n)
{
    for (; *s != '\0' && n > 0; s++, n--) {
        if (*s == c)
            return 1;
    }
    return 0;
}

char *strncat(char *s1, const char *s2, size_t n)
{
    char *s = s1;
    /* Find the end of S1.  */
    s1 += strlen(s1);
    size_t ss = strnlen(s2, n);
    s1[ss] = '\0';
    memcpy(s1, s2, ss);
    return s;
}

char *strrchr(const char *cp, int ch)
{
    char *save;
    char c;

    for (save = (char *) 0; (c = *cp); cp++) {
        if (c == ch)
            save = (char *) cp;
    }

    return save;
}

size_t strlcpy(char *dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';        /* NUL-terminate dst */
        while (*s++);
    }

    return (s - src - 1);    /* count does not include NUL */
}

static unsigned char charmap[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
        0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
        0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
        0xc0, 0xe1, 0xe2, 0xe3, 0xe4, 0xc5, 0xe6, 0xe7,
        0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
        0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

int strcasecmp(register const char *s1, register const char *s2)
{
    register unsigned char u1, u2;

    for ( ; ; s1++, s2++) {
        u1 = (unsigned char) *s1;
        u2 = (unsigned char) *s2;
        if ((u1 == '\0') || (charmap[u1] != charmap[u2])) {
            break;
        }
    }
    return charmap[u1] - charmap[u2];
}

int strncasecmp(register const char *s1, register const char *s2, size_t length)
{
    register unsigned char u1, u2;

    for (; length != 0; length--, s1++, s2++) {
        u1 = (unsigned char) *s1;
        u2 = (unsigned char) *s2;
        if (charmap[u1] != charmap[u2]) {
            return charmap[u1] - charmap[u2];
        }
        if (u1 == '\0') {
            return 0;
        }
    }
    return 0;
}

const char *strcasestr(register const char *string, const char *substring)
{
    register const char *a, *b;
    register unsigned char u1, u2;

    /* First scan quickly through the two strings looking for a
     * single-character match.  When it's found, then compare the
     * rest of the substring.
     */

    b = substring;
    if (*b == 0) {
        return string;
    }
    for (; *string != 0; string += 1) {
        u1 = (unsigned char) *string;
        u2 = (unsigned char) *b;
        if (charmap[u1] != charmap[u2]) {
            continue;
        }
        a = string;
        while (1) {
            if (*b == 0) {
                return string;
            }
            u1 = (unsigned char) *a;
            u2 = (unsigned char) *b;
            if (charmap[u1] != charmap[u2]) {
                break;
            }
            b++;
            a++;
        }
        b = substring;
    }
    return NULL;
}