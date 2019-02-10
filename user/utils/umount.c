//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <syscallw.h>
#include "kstd.h"

void help() {
    printf("umount <utils point>\n");
}

int main(int ac, char **av) {
    if (ac != 2) {
        help();
        return 1;
    }

    printf("umount: %d\n", umount(av[1][0]));
    return 0;
}