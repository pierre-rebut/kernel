//
// Created by rebut_p on 14/02/19.
//

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

int main(int ac, char **av)
{
    if (ac == 1) {
        printf("usage: touch [FILE]...\n");
        return -1;
    }

    int res = 0;
    for (int i = 1; i < ac; i++) {
        int tmp = mkfile(av[i], S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (tmp < 0) {
            res = 1;
            printf("touch: %s: %s\n", av[i], strerror(errno));
        }
    }

    return res;
}