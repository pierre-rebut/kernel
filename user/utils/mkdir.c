//
// Created by rebut_p on 24/02/19.
//

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

int main(int ac, char **av) {
    if (ac < 2) {
        printf("usage: mkdir [DIRECTORY]...\n");
        return 1;
    }

    int res = 0;
    for (int i = 1; i < ac; i++) {
        int tmp = mkdir(av[i], S_IRWXU | S_IRGRP | S_IROTH);
        if (tmp < 0) {
            printf("mkdir: can not create directory %s: %s\n", av[i], strerror(errno));
            res = 1;
        }
    }

    return res;
}