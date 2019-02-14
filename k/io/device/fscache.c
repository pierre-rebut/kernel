//
// Created by rebut_p on 29/12/18.
//

#include <sys/allocator.h>
#include <string.h>
#include <include/kstdio.h>
#include "fscache.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

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
    tmpCache->nbBlock = 0;
    tmpCache->lastBlock = NULL;

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
                if (fsCache->lastBlock == tmpBlock) {
                    fsCache->lastBlock = tmpBlock->prev;
                }

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

static struct FsCacheBlock *createFsCacheBlock(struct FsCache *fsCache, void *data, int nblocks, int offset) {
    LOG("[fsCache] create new block cache: %d\n", offset);

    struct FsCacheBlock *tmpBlock = kmalloc(sizeof(struct FsCacheBlock), 0, "newTmpBlock");
    if (tmpBlock == NULL)
        return NULL;

    tmpBlock->data = data;
    tmpBlock->nblocks = nblocks;

    tmpBlock->offset = offset;
    tmpBlock->next = fsCache->dataBlock;
    tmpBlock->prev = NULL;

    if (fsCache->dataBlock)
        fsCache->dataBlock->prev = tmpBlock;

    fsCache->dataBlock = tmpBlock;
    if (fsCache->lastBlock == NULL)
        fsCache->lastBlock = tmpBlock;

    if (fsCache->nbBlock < FSCACHE_NB_BLOCK_LIMIT)
        fsCache->nbBlock++;
    else {
        LOG("[fsCache] remove block upon limit\n");
        struct FsCacheBlock *tmp = fsCache->lastBlock;
        fsCache->lastBlock = tmp->prev;
        fsCache->lastBlock->next = NULL;

        if (tmp->updated == 1) {
            deviceWrite(fsCache->device, tmp->data, tmp->nblocks, tmp->offset);
        }

        kfree(tmp->data);
        kfree(tmp);
    }
    return tmpBlock;
}

int fsCacheRead(struct Device *device, void *buffer, int nblocks, int offset) {
    struct FsCache *fsCache = findOrCreateFsCache(device);
    if (fsCache == NULL)
        return -1;

    struct FsCacheBlock *fsCacheBlock = findFsCacheBlock(fsCache, offset);
    if (fsCacheBlock == NULL) {
        void *data = kmalloc((u32) nblocks * fsCache->device->blockSize, 0, "newDataBlock");
        if (data == NULL)
            return -1;

        if (deviceRead(fsCache->device, data, nblocks, offset) == -1) {
            kfree(data);
            return -1;
        }

        fsCacheBlock = createFsCacheBlock(fsCache, data, nblocks, offset);
    }

    if (fsCacheBlock == NULL)
        return -1;

    fsCacheBlock->updated = 0;
    memcpy(buffer, fsCacheBlock->data, (u32) nblocks * device->blockSize);
    return nblocks;
}

int fsCacheWrite(struct Device *device, const void *buffer, int nblocks, int offset) {
    struct FsCache *fsCache = findOrCreateFsCache(device);
    if (fsCache == NULL)
        return -1;

    void *data;

    struct FsCacheBlock *fsCacheBlock = findFsCacheBlock(fsCache, offset);
    if (fsCacheBlock == NULL) {
        data = kmalloc((u32) nblocks * fsCache->device->blockSize, 0, "newDataBlock");
        if (data == NULL)
            return -1;

        fsCacheBlock = createFsCacheBlock(fsCache, data, nblocks, offset);
    } else {
        klog("[fsCache] cache found\n");
        data = fsCacheBlock->data;
    }

    fsCacheBlock->updated = 1;
    memcpy(data, buffer, (u32) nblocks * fsCache->device->blockSize);
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

        if (tmp->updated == 1) {
            deviceWrite(device, tmp->data, tmp->nblocks, tmp->offset);
        }

        kfree(tmp->data);
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

static void fsCacheSyncDevice(struct FsCache *fsCache) {
    struct FsCacheBlock *tmpBlock = fsCache->dataBlock;

    while (tmpBlock != NULL) {
        if (tmpBlock->updated == 1) {
            tmpBlock->updated = 0;
            deviceWrite(fsCache->device, tmpBlock->data, tmpBlock->nblocks, tmpBlock->offset);
        }

        tmpBlock = tmpBlock->next;
    }
}

void fsCacheSync() {
    struct FsCache *tmpCache = fsCacheList;

    while (tmpCache != NULL) {
        fsCacheSyncDevice(tmpCache);
        tmpCache = tmpCache->next;
    }
}