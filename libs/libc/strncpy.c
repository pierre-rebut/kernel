//
// Created by rebut_p on 14/12/18.
//

#include <string.h>

char *strncpy(char *dst, char const *src, size_t n) {
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
