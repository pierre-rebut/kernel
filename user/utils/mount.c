//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <kstd.h>
#include <string.h>
#include <stdlib.h>

void help() {
    printf("mount <mounttype> <fs type> <utils point> <data>\n");
    printf("<fs type> : kfs / proc / iso\n");
    printf("<data> : file / unit atapi\n");
}

int main(int ac, char **av) {
    if (ac != 5) {
        help();
        return 1;
    }

    if (strcmp(av[1], "--file") == 0)
        printf("mount %s: %d\n", av[4], mount(av[3][0], av[2], av[4]));
    else if (strcmp(av[1], "--atapi") == 0) {
        int unit = atoi(av[4]);
        if (unit < 0) {
            help();
            return 1;
        }

        printf("mount atapi %d: %d\n", unit, mount2(av[3][0], av[2], unit));
    } else
        help();
    return 0;
}