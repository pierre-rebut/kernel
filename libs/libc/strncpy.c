//
// Created by rebut_p on 30/09/18.
//
#include <stddef.h>

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != 0; i++) {
        dest[i] = src[i];
    }
    return (dest);
}
