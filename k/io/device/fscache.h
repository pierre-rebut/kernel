//
// Created by rebut_p on 29/12/18.
//

#ifndef KERNEL_FSCACHE_H
#define KERNEL_FSCACHE_H

#include <k/types.h>
#include "device.h"

struct FsCacheBlock {
    int offset;
    void *data;

    struct FsCacheBlock *next;
    struct FsCacheBlock *prev;
};

struct FsCache {
    struct Device *device;
    struct FsCacheBlock *dataBlock;

    struct FsCache *next;
    struct FsCache *prev;
};

int fsCacheRead(struct Device *device, void *buffer, int nblocks, int offset);
int fsCacheWrite(struct Device *device, const void *buffer, int nblocks, int offset);
int fsCacheFlush(struct Device *device);

#endif //KERNEL_FSCACHE_H
