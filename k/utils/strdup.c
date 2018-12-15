//
// Created by rebut_p on 15/12/18.
//
#include <string.h>
#include <sys/allocator.h>

char *strdup(const char *str) {
    if (!str)
        return NULL;

    u32 size = strlen(str);
    char *newStr = kmalloc(sizeof(char) * size, 0, "strdup");
    if (!newStr)
        return NULL;

    u32 i;
    for (i = 0; i < size; i++)
        newStr[i] = str[i];
    newStr[i] = '\0';
    return newStr;
}
