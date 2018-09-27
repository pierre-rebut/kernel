//
// Created by rebut_p on 23/09/18.
//

#ifndef KERNEL_EPITA_KFILESYSTEM_H
#define KERNEL_EPITA_KFILESYSTEM_H

#include "multiboot.h"

#include <k/kfs.h>
#include <k/kstd.h>

void initKFileSystem(module_t *module);

int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t size);
off_t seek(int fd, off_t offset, int whence);
int close(int fd);

u32 length(const char *pathname);

void listFiles();

struct file_entry {
    char used : 1;
    u32 dataIndex;
    u32 blockIndex;
    u32 iblockIndex;
    u32 *d_blks;

    struct kfs_block *block;
    struct kfs_iblock *iblock;

    int flags;
    struct kfs_inode *node;
} __attribute__((packed));

#endif //KERNEL_EPITA_KFILESYSTEM_H
