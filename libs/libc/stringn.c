//
// Created by rebut_p on 17/12/18.
//

#include <string.h>

int strncmp(const char *s1, const char *s2, u32 n) {
    for (u32 i = 0; i < n; ++i)
        if (s1[i] == '\0' || s1[i] != s2[i])
            return s1[i] - s2[i];

    return 0;
}

char *strncpy(char *dst, char const *src, u32 n) {
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

u32 strnlen(const char *s, u32 maxlen) {
    register u32 i = 0;
    for (; i < maxlen; ++i)
        if (!s[i])
            return i;
    return i;
}

int strncontain(const char *s, char c, u32 n) {
    for (; *s != '\0' && n > 0; s++, n--) {
        if (*s == c)
            return 1;
    }
    return 0;
}

char *strncat(char *s1, const char *s2, size_t n) {
    char *s = s1;
    /* Find the end of S1.  */
    s1 += strlen(s1);
    size_t ss = strnlen(s2, n);
    s1[ss] = '\0';
    memcpy(s1, s2, ss);
    return s;
}

char *strrchr(const char *cp, int ch) {
    char *save;
    char c;

    for (save = (char *) 0; (c = *cp); cp++) {
        if (c == ch)
            save = (char *) cp;
    }

    return save;
}

size_t strlcpy(char *dst, const char *src, size_t siz) {
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