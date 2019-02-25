//
// Created by rebut_p on 26/02/19.
//

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int ac, char **av) {
    if (ac < 2) {
        printf("usage: rm [FILE]...\n");
        return 1;
    }

    int res = 0;
    for (int i = 1; i < ac; i++) {
        int tmp = unlink(av[i]);
        if (tmp < 0) {
            printf("rm: can not remove %s: %s\n", av[i], strerror(errno));
            res = 1;
        }
    }

    return res;
}