//
// Created by rebut_p on 21/12/18.
//

#include <stdio.h>
#include <sys/mount.h>
#include <errno.h>

int main(int ac, char **av)
{
    if (ac != 2) {
        printf("umount <mount point>\n");
        return 1;
    }

    int res = umount(av[1]);
    if (res < 0)
        printf("umount: %s: %s\n", av[1], strerror(errno));
    return 0;
}