//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <kstd.h>

void help() {
    printf("utils <fs type> <utils point> <file>\n");
    printf("<fs type> : kfs\n");
}

int main(int ac, char **av) {
    if (ac != 4) {
        help();
        return 1;
    }

    printf("utils: %d\n", mount(av[2][0], av[1], av[3]));
    return 0;
}