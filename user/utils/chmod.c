//
// Created by rebut_p on 24/02/19.
//

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

int getmode(const char *str, mode_t *mode) {
    if (strlen(str) != 3) {
        printf("chmod: bad octal-mode value (len must be 3)\n");
        return 1;
    }

    for (int i = 0; i < 3; i++) {
        if (str[i] < '0' || str[i] > '7') {
            printf("chmod: bad octal-mode value (mode must be between 0 and 7)\n");
            return 1;
        }

        *mode += (str[i] - 48) << (6 - (3 * i));
    }

    return 0;
}

int main(int ac, char **av) {
    if (ac < 3) {
        printf("usage: chmod <mode> [FILE]...\n");
        return 1;
    }

    mode_t mode = 0;

    if (getmode(av[1], &mode) != 0)
        return 1;

    int res = 0;
    for (int i = 2; i < ac; i++) {
        int tmp = chmod(av[i], mode);
        if (tmp < 0) {
            res = 1;
            printf("chmod: %s: %s\n", av[i], strerror(errno));
        }
    }

    return res;
}