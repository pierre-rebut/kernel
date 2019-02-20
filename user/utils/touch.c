//
// Created by rebut_p on 14/02/19.
//

#include <stdio.h>
#include <unistd.h>

int main(int ac, char **av)
{
    if (ac == 1) {
        printf("%s: file(s) missing\n", av[0]);
        return -1;
    }

    for (int i = 1; i < ac; i++)
        mkfile(av[i], 0);

    return 0;
}