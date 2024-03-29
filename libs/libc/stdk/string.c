//
// Created by rebut_p on 17/12/18.
//

#include <kernel/string.h>

int strcontain(const char *s1, char c)
{
    for (; *s1 != '\0'; s1++) {
        if (*s1 == c)
            return 1;
    }
    return 0;
}

int strcmp(const char *s1, const char *s2)
{
    for (; *s1 == *s2 && *s1 != '\0'; s1++, s2++)
        continue;

    return *s1 - *s2;
}

char *strcpy(char *dest, const char *src)
{
    char *p = dest;

    while (*src)
        *p++ = *src++;

    *p = '\0';

    return dest;
}

u32 strlen(const char *s)
{
    register const char *p = s;

    while (*p)
        p++;

    return (p - s);
}

char *strchr(const char *p, int ch)
{
    char c = (char) ch;
    for (;; ++p) {
        if (*p == c)
            return ((char *) p);
        if (*p == '\0')
            return (NULL);
    }
}

u32 strcspn(const char *s1, register const char *s2)
{
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

char *strtok(char *s, const char *delim)
{
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

void strtoupper(char *str)
{
    while (*str) {
        if (*str >= 'a' && *str <= 'z')
            *str = *str - 32;
        str++;
    }
}

u32 str_backspace(char *str, char c, char **file)
{
    size_t i = strlen(str);
    i--;
    while (i) {
        i--;
        if (str[i] == c) {
            str[i] = 0;
            *file = str + i + 1;
            return 1;
        }
    }
    *file = str;
    return 0;
}

u32 strsplit(char *str, char delim)
{
    size_t n = 0;
    u32 i = 0;
    while (str[i]) {
        if (str[i] == delim) {
            str[i] = 0;
            n++;
        }
        i++;
    }
    n++;
    return n;
}

u32 str_begins_with(const char *str, const char *with)
{
    size_t j = strlen(with);
    size_t i = 0;
    size_t ret = 1;
    while (with[j] != 0) {
        if (str[i] != with[i]) {
            ret = 0;
            break;
        }
        j--;
        i++;
    }
    return ret;
}

const char *strstr(register const char *string, char *substring)
{
    register const char *a, *b;

    /* First scan quickly through the two strings looking for a
     * single-character match.  When it's found, then compare the
     * rest of the substring.
     */

    b = substring;
    if (*b == 0) {
        return string;
    }
    for (; *string != 0; string += 1) {
        if (*string != *b) {
            continue;
        }
        a = string;
        while (1) {
            if (*b == 0) {
                return string;
            }
            if (*a++ != *b++) {
                break;
            }
        }
        b = substring;
    }
    return NULL;
}

void strmode(register mode_t mode, register char *p)
{
    /* print type */
    switch (mode & S_IFMT) {
        case S_IFDIR:            /* directory */
            *p++ = 'd';
            break;
        case S_IFCHR:            /* character special */
            *p++ = 'c';
            break;
        case S_IFBLK:            /* block special */
            *p++ = 'b';
            break;
        case S_IFREG:            /* regular */
            *p++ = '-';
            break;
        case S_IFLNK:            /* symbolic link */
            *p++ = 'l';
            break;
        case S_IFSOCK:            /* socket */
            *p++ = 's';
            break;
#ifdef S_IFIFO
        case S_IFIFO:            /* fifo */
            *p++ = 'p';
            break;
#endif
#ifdef S_IFWHT
        case S_IFWHT:			/* whiteout */
            *p++ = 'w';
            break;
#endif
        default:            /* unknown */
            *p++ = '?';
            break;
    }
    /* usr */
    if (mode & S_IRUSR)
        *p++ = 'r';
    else
        *p++ = '-';
    if (mode & S_IWUSR)
        *p++ = 'w';
    else
        *p++ = '-';
    switch (mode & (S_IXUSR | S_ISUID)) {
        case S_IXUSR:
            *p++ = 'x';
            break;
        case S_ISUID:
            *p++ = 'S';
            break;
        case S_IXUSR | S_ISUID:
            *p++ = 's';
            break;
        default:
            *p++ = '-';
    }
    /* group */
    if (mode & S_IRGRP)
        *p++ = 'r';
    else
        *p++ = '-';
    if (mode & S_IWGRP)
        *p++ = 'w';
    else
        *p++ = '-';
    switch (mode & (S_IXGRP | S_ISGID)) {
        case S_IXGRP:
            *p++ = 'x';
            break;
        case S_ISGID:
            *p++ = 'S';
            break;
        case S_IXGRP | S_ISGID:
            *p++ = 's';
            break;
        default:
            *p++ = '-';
    }
    /* other */
    if (mode & S_IROTH)
        *p++ = 'r';
    else
        *p++ = '-';
    if (mode & S_IWOTH)
        *p++ = 'w';
    else
        *p++ = '-';
    switch (mode & (S_IXOTH | S_ISVTX)) {
        case S_IXOTH:
            *p++ = 'x';
            break;
        case S_ISVTX:
            *p++ = 'T';
            break;
        case S_IXOTH | S_ISVTX:
            *p++ = 't';
            break;
        default:
            *p++ = '-';
    }
    *p++ = ' ';        /* will be a '+' if ACL's implemented */
    *p = '\0';
}

char *strpbrk(register char *string, char *chars)
{
    register char c, *p;

    for (c = *string; c != 0; string++, c = *string) {
        for (p = chars; *p != 0; p++) {
            if (c == *p) {
                return string;
            }
        }
    }
    return 0;
}

char *dirname(char *path)
{
    static const char dot[] = ".";
    char *last_slash;
    /* Find last '/'.  */
    last_slash = path != NULL ? strrchr(path, '/') : NULL;

    if (last_slash == path)
        /* The last slash is the first character in the string.  We have to
       return "/".  */
        ++last_slash;
    else if (last_slash != NULL && last_slash[1] == '\0')
        /* The '/' is the last character, we have to look further.  */
        last_slash = memchr(path, last_slash - path, '/');

    if (last_slash != NULL)
        /* Terminate the path.  */
        last_slash[0] = '\0';
    else
        /* This assignment is ill-designed but the XPG specs require to
       return a string containing "." in any case no directory part is
       found and so a static and constant string is required.  */
        path = (char *) dot;

    return path;
}

char *basename (const char *filename)
{
    char *p = strrchr (filename, '/');
    return p ? p + 1 : (char *) filename;
}

