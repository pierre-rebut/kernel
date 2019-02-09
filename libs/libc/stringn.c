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
    u32 i = 0;
    for (; i < maxlen; ++i)
        if (!s[i])
            return i;
    return i;
}