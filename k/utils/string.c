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

char *strcat(char *begin, char *end) {
    char *str;
    int i;
    int j;

    j = 0;
    i = 0;
    str = NULL;
    if (begin != NULL && end != NULL) {
        str = kmalloc(sizeof(char) * (strlen(begin) + strlen(end) + 1), 0, "strcat");
        while (begin[i] != '\0') {
            str[i] = begin[i];
            i++;
        }
        while (end[j] != '\0') {
            str[i] = end[j];
            j++;
            i++;
        }
        str[i] = '\0';
    }
    return (str);
}