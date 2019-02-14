//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include "kstd.h"
#include <string.h>
#include <syscallw.h>
#include "../../libs/include/stdlib.h"

void help() {
    printf("mount <mounttype> <fs type> <arg> <mount point>\n");
    printf("<mounttype> : -f / -d\n");
    printf("<fs type> : kfs / procfs / devfs / ext2fs\n");
    printf("<arg> : file / ata unit\n");
}

int main(int ac, char **av) {
    if (ac != 5) {
        help();
        return 1;
    }

    if (strcmp(av[1], "-f") == 0)
        printf("mount %s -> %s: %d\n", av[3], av[4], mount(av[4], av[2], (u32) av[3]));
    else if (strcmp(av[1], "-d") == 0) {
        int unit = atoi(av[3]);
        if (unit < 0) {
            help();
            return 1;
        }

        printf("mount device %d -> %s: %d\n", unit, av[4], mount(av[4], av[2], (u32) unit));
    } else
        help();
    return 0;
}