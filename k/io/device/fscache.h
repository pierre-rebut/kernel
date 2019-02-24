//
// Created by rebut_p on 29/12/18.
//

#ifndef KERNEL_FSCACHE_H
#define KERNEL_FSCACHE_H

#include <stddef.h>
#include "device.h"

#define FSCACHE_NB_BLOCK_LIMIT 2000

struct FsCacheBlock
{
    u32 offset;
    int nblocks;
    char updated;

    void *data;
    struct FsCacheBlock *next;
    struct FsCacheBlock *prev;
};

struct FsCache
{
    struct Device *device;
    struct FsCacheBlock *dataBlock;

    struct FsCache *next;
    struct FsCache *prev;

    u32 nbBlock;
    struct FsCacheBlock *lastBlock;
};

int fsCacheRead(struct Device *device, void *buffer, int nblocks, u32 offset);

int fsCacheWrite(struct Device *device, const void *buffer, int nblocks, u32 offset);

int fsCacheFlush(struct Device *device);

void fsCacheSync();

#endif //KERNEL_FSCACHE_H
