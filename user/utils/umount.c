//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <unistd.h>
#include "kstd.h"

void help() {
    printf("umount <mount point>\n");
}

int main(int ac, char **av) {
    if (ac != 2) {
        help();
        return 1;
    }

    printf("umount: %d\n", umount(av[1]));
    return 0;
}