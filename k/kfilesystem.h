//
// Created by rebut_p on 23/09/18.
//

#ifndef KERNEL_EPITA_FILESYSTEM_H
#define KERNEL_EPITA_FILESYSTEM_H

#include "multiboot.h"

#include <k/kfs.h>
#include <k/kstd.h>

void initKFileSystem(module_t *module);

int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t size);
off_t seek(int fd, off_t offset, int whence);
int close(int fd);

void listFiles();

struct file_entry {
    char used : 1;
    off_t offset;
    int flags;
    struct kfs_inode *node;
};

#endif //KERNEL_EPITA_FILESYSTEM_H
