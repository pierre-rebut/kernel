//
// Created by rebut_p on 23/09/18.
//

#ifndef KERNEL_EPITA_KFILESYSTEM_H
#define KERNEL_EPITA_KFILESYSTEM_H

#include <multiboot.h>

#include <k/kfs.h>
#include <k/kstd.h>
#include "filesystem.h"

void initKFileSystem(module_t *module);

struct FileDescriptor *kfsOpen(const char *pathname, int flags);

s32 kfsRead(void *entryData, void *buf, u32 size);
off_t kfsSeek(void *entryData, off_t offset, int whence);
int kfsClose(void *entryData);

u32 kfsLengthOfFile(const char *pathname);

void kfsListFiles();

struct file_entry {
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
