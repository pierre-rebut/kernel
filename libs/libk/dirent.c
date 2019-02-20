//
// Created by rebut_p on 16/02/19.
//

#include <dirent.h>
#include <string.h>
#include <err.h>
#include <alloc.h>

#include "syscalls.h"

DIR *opendir(const char *name)
{
    warn("[dirent] open dir: %s\n", name);
    int fd = syscall1(SYSCALL_OPENDIR, (u32) name);
    if (fd < 0) {
        return NULL;
    }

    DIR *dirp = malloc(sizeof(DIR));
    if (dirp == NULL)
        return NULL;

    memset(dirp, 0, sizeof(DIR));
    dirp->fd = fd;

    return dirp;
}

int closedir(DIR *dirp)
{
    warn("[dirent] close dir\n");
    if (dirp == NULL)
        return -1;

    int res = syscall1(SYSCALL_CLOSEDIR, (u32) dirp->fd);
    if (res < 0) {
        res = -1;
    }

    free(dirp);
    return res;
}

struct dirent *readdir(DIR *dirp)
{
    if (dirp == NULL)
        return NULL;

    warn("[dirent] readdir: %d / %d\n", dirp->offset, dirp->size);
    if (dirp->offset >= dirp->size) {
        int size = syscall3(SYSCALL_READDIR, (u32) dirp->fd, (u32) dirp->block, dirp->nblock);
        if (size <= 0)
            return NULL;

        dirp->nblock += 1;
        dirp->offset = 0;
        dirp->size = (u32) size;
    }

    struct dirent *tmp = (struct dirent *) dirp->block;
    memcpy(&(dirp->data), tmp + dirp->offset, sizeof(struct dirent));
    dirp->offset += 1;
    warn("[dirent] readdir end\n");

    return &(dirp->data);
}