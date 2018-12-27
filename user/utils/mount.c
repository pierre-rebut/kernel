//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <kstd.h>
#include <string.h>
#include <stdlib.h>

void help() {
    printf("mount <mounttype> <fs type> <mount point> <arg>\n");
    printf("<mounttype> : --file / --ata\n");
    printf("<fs type> : kfs / procfs / devfs / ext2fs\n");
    printf("<arg> : file / ata unit\n");
}

int main(int ac, char **av) {
    if (ac != 5) {
        help();
        return 1;
    }

    if (strcmp(av[1], "--file") == 0)
        printf("mount %s: %d\n", av[4], mount(av[3][0], av[2], (u32) av[4]));
    else if (strcmp(av[1], "--ata") == 0) {
        int unit = atoi(av[4]);
        if (unit < 0) {
            help();
            return 1;
        }

        printf("mount atapi %d: %d\n", unit, mount(av[3][0], av[2], (u32) unit));
    } else
        help();
    return 0;
}