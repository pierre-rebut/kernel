//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <kstd.h>

void help() {
    printf("mount <fs type> <mount point> <file>\n");
    printf("<fs type> : kfs\n");
}

int main(int ac, char **av) {
    if (ac != 4) {
        help();
        return 1;
    }

    printf("mount: %d\n", mount(av[2][0], av[1], av[3]));
    return 0;
}