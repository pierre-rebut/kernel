//
// Created by rebut_p on 29/12/18.
//

#ifndef KERNEL_FSCACHE_H
#define KERNEL_FSCACHE_H

#define FSCACHE_NB_BLOCK_LIMIT 2000

#include <k/ktypes.h>
#include "device.h"

struct FsCacheBlock {
    int offset;
    void *data;
    int nblocks;

    char updated;

    struct FsCacheBlock *next;
    struct FsCacheBlock *prev;
};

struct FsCache {
    struct Device *device;
    struct FsCacheBlock *dataBlock;

    struct FsCache *next;
    struct FsCache *prev;

    u32 nbBlock;
    struct FsCacheBlock *lastBlock;
};

int fsCacheRead(struct Device *device, void *buffer, int nblocks, int offset);
int fsCacheWrite(struct Device *device, const void *buffer, int nblocks, int offset);
int fsCacheFlush(struct Device *device);

#endif //KERNEL_FSCACHE_H
