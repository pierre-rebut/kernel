//
// Created by rebut_p on 17/12/18.
//

#include <string.h>
#include <sys/allocator.h>

char *strdup(const char *s) {
    if (!s)
        return NULL;

    char *r = NULL;
    char *p = NULL;

    r = kmalloc(strlen(s) + 1, 0, "strdup");
    if (!r)
        return NULL;

    for (p = r; *s != '\0'; s++, p++)
        *p = *s;

    *p = '\0';

    return (r);
}

int strcmp(const char *s1, const char *s2) {
    for (; *s1 == *s2 && *s1 != '\0'; s1++, s2++)
        continue;

    return *s1 - *s2;
}

char *strcpy(char *dest, const char *src) {
    char *p = dest;

    while (*src)
        *p++ = *src++;

    *p = '\0';

    return dest;
}

u32 strlen(const char *s) {
    const char *p = s;

    while (*p)
        p++;

    return (p - s);
}

char *strchr(const char *p, int ch) {
    char c = (char) ch;
    for (;; ++p) {
        if (*p == c)
            return ((char *) p);
        if (*p == '\0')
            return (NULL);
    }
}

u32 strcspn(const char *s1, register const char *s2) {
    register const char *p, *spanp;
    register char c, sc;

    for (p = s1;;) {
        c = *p++;
        spanp = s2;
        do {
            if ((sc = *spanp++) == c)
                return (p - 1 - s1);
        } while (sc != 0);
    }
}

char *strtok(char *s, const char *delim) {
    static char *lasts;
    register int ch;

    if (s == 0)
        s = lasts;
    do {
        if ((ch = *s++) == '\0')
            return 0;
    } while (strchr(delim, ch));
    --s;
    lasts = s + strcspn(s, delim);
    if (*lasts != 0)
        *lasts++ = 0;
    return s;
}

char isspace(char c) {
    if (c == ' '
        || c == '\f'
        || c == '\n'
        || c == '\r'
        || c == '\t'
        || c == '\v')
        return 1;

    return 0;
}
