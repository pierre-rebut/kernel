//
// Created by rebut_p on 22/12/18.
//

#include <stdio.h>
#include "../../libs/libk/include/syscallw.h"
#include "kstd.h"

int main(int ac, char **av) {
    if (ac != 2) {
        printf("cat <file>\n");
        return 1;
    }

    int fd = open(av[1], O_RDONLY);
    if (fd == -1) {
        printf("cat %s: can not open file\n", av[1]);
        return 1;
    }

    char buffer[4096];
    int size;

    while ((size = read(fd, buffer, 4095)) > 0) {
        //printf("test: %d\n", size);
        buffer[size] = '\0';
        write(1, buffer, (u32) size);
    }

    close(fd);
    return 0;
}