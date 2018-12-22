#include <string.h>

int write(int fd, const char *s, size_t nb);

int puts(const char *s) {
    return write(1, s, strlen(s));
}

int putErrors(const char *s) {
    return write(2, s, strlen(s));
}
