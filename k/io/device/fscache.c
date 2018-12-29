//
// Created by rebut_p on 29/12/18.
//

#include <sys/allocator.h>
#include <string.h>
#include <include/stdio.h>
#include "fscache.h"

#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
//#define LOG(x, ...)

struct FsCache *fsCacheList = NULL;

static struct FsCache *findOrCreateFsCache(struct Device *device) {
    struct FsCache *tmpCache = fsCacheList;

    while (tmpCache != NULL) {
        if (tmpCache->device == device) {
            return tmpCache;
        }

        tmpCache = tmpCache->next;
    }

    LOG("[fsCache] create new fsCache\n");
    tmpCache = kmalloc(sizeof(struct FsCache), 0, "newFsCache");
    if (tmpCache == NULL)
        return NULL;

    tmpCache->device = device;
    tmpCache->prev = NULL;
    tmpCache->next = fsCacheList;
    tmpCache->dataBlock = NULL;
    if (fsCacheList)
        fsCacheList->prev = tmpCache;
    fsCacheList = tmpCache;
    return tmpCache;
}

static struct FsCache *findFsCache(struct Device *device) {
    struct FsCache *tmpCache = fsCacheList;

    while (tmpCache != NULL) {
        if (tmpCache->device == device)
            return tmpCache;

        tmpCache = tmpCache->next;
    }

    return NULL;
}

static struct FsCacheBlock *findFsCacheBlock(struct FsCache *fsCache, int offset) {
    struct FsCacheBlock *tmpBlock = fsCache->dataBlock;

    while (tmpBlock != NULL) {
        if (tmpBlock->offset == offset) {

            LOG("[fsCache] cache found %d\n", offset);

            if (fsCache->dataBlock != tmpBlock) {
                if (tmpBlock->next)
                    tmpBlock->next->prev = tmpBlock->prev;
                if (tmpBlock->prev)
                    tmpBlock->prev->next = tmpBlock->next;

                fsCache->dataBlock->prev = tmpBlock;
                tmpBlock->next = fsCache->dataBlock;
                tmpBlock->prev = NULL;
                fsCache->dataBlock = tmpBlock;
            }

            return tmpBlock;
        }

        tmpBlock = tmpBlock->next;
    }

    return NULL;
}

static struct FsCacheBlock *createAndReadDevice(struct FsCache *fsCache, int nblocks, int offset) {
    LOG("[fsCache] create new block cache: %d\n", offset);

    struct FsCacheBlock *tmpBlock = kmalloc(sizeof(struct FsCacheBlock), 0, "newTmpBlock");
    if (tmpBlock == NULL)
        return NULL;

    tmpBlock->data = kmalloc((u32)nblocks * fsCache->device->blockSize, 0, "newDataBlock");
    if (tmpBlock->data == NULL)
        goto createAndReadDeviceFailure;

    if (deviceRead(fsCache->device, tmpBlock->data, nblocks, offset) == -1)
        goto createAndReadDeviceFailure;

    tmpBlock->offset = offset;
    tmpBlock->next = fsCache->dataBlock;
    tmpBlock->prev = NULL;

    if (fsCache->dataBlock)
        fsCache->dataBlock->prev = tmpBlock;

    fsCache->dataBlock = tmpBlock;
    return tmpBlock;

    createAndReadDeviceFailure:
    kfree(tmpBlock->data);
    kfree(tmpBlock);
    return NULL;
}

int fsCacheRead(struct Device *device, void *buffer, int nblocks, int offset) {
    struct FsCache *fsCache = findOrCreateFsCache(device);
    if (fsCache == NULL)
        return -1;

    struct FsCacheBlock *fsCacheBlock = findFsCacheBlock(fsCache, offset);
    if (fsCacheBlock == NULL)
        fsCacheBlock = createAndReadDevice(fsCache,nblocks, offset);

    if (fsCacheBlock == NULL)
        return -1;

    memcpy(buffer, fsCacheBlock->data, (u32)nblocks * device->blockSize);
    return nblocks;
}

int fsCacheFlush(struct Device *device) {
    struct FsCache *fsCache = findFsCache(device);
    if (fsCache == NULL)
        return 0;

    struct FsCacheBlock *tmpBlock = fsCache->dataBlock;
    while (tmpBlock) {
        struct FsCacheBlock *tmp = tmpBlock;
        tmpBlock = tmpBlock->next;
        kfree(tmp);
    }

    if (fsCache->next)
        fsCache->next->prev = fsCache->prev;
    if (fsCache->prev)
        fsCache->prev->next = fsCache->next;
    else
        fsCacheList = fsCache->next;

    kfree(fsCache);
    return 0;
}