//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include "kstd.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../../libs/include/stdlib.h"

void help() {
    printf("mount <fs type> <arg> <mount point>\n");
    printf("<fs type> : kfs / procfs / devfs / ext2fs\n");
}

int main(int ac, char **av) {
    if (ac != 4) {
        help();
        return 1;
    }

    printf("mount %s -> %s: %d\n", av[2], av[3], mount(av[1], av[2], av[3]));
    if (errno)
        strerror(errno);
    return 0;
}