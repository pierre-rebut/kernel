//
// Created by rebut_p on 24/02/19.
//

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int ac, char **av) {
    if (ac != 3) {
        printf("usage: link [FILE1] [FILE2]\n");
        return 1;
    }

    int res = link(av[1], av[2]);
    if (res < 0) {
        printf("link: can not link %s on %s: %s\n", av[2], av[1], strerror(errno));
        return 1;
    }

    return 0;
}