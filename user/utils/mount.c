//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <errno.h>
#include <sys/mount.h>

void help()
{
    printf("mount <fs type> <arg> <mount point>\n");
    printf("<fs type> : kfs / procfs / devfs / ext2fs\n");
}

int main(int ac, char **av)
{
    if (ac != 4) {
        help();
        return 1;
    }

    int res = mount(av[1], av[2], av[3]);
    if (res < 0)
        printf("mount: %s (%s): %s\n", av[2], av[1], strerror(errno));

    return res;
}