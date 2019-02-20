//
// Created by rebut_p on 15/11/18.
//

#include "allocator.h"
#include "paging.h"
#include "physical-memory.h"
#include <kstdio.h>
#include <string.h>

typedef struct
{
    u32 size;
    u32 number;
    char reserved;
    char comment[21];
} __attribute__((packed)) region_t;


static region_t *regions = 0;
static u32 regionCount = 0;
static u32 regionMaxCount = 0;
static u32 firstFreeRegion = 0;
static void *firstFreeAddr = (void *) KERNEL_HEAP_START;
static u8 *const heapStart = (void *) KERNEL_HEAP_START;
static u32 heapSize = 0;
static const u32 HEAP_MIN_GROWTH = 0x10000;

static struct Mutex mutex = mutexInit();

static void *tmpMemAlloc(size_t size, u32 alignment);


int initAllocator(void)
{
    regions = tmpMemAlloc(0, 0);

    regionCount = 0;
    regionMaxCount = (PLACEMENT_END - (u32) regions) / sizeof(region_t);
    return 0;
}

static char allocHeapGrow(size_t size, u8 *heapEnd)
{
    if ((regionCount > 0) && regions[regionCount - 1].reserved && (regionCount >= regionMaxCount))
        return (0);

    mutexLock(&mutex);
    if (pagingAlloc(kernelPageDirectory, heapEnd, size, MEM_WRITE)) {
        mutexUnlock(&mutex);
        return (0);
    }

    if ((regionCount > 0) && !regions[regionCount - 1].reserved)
        regions[regionCount - 1].size += size;
    else {
        regions[regionCount].reserved = 0;
        regions[regionCount].size = size;
        regions[regionCount].number = 0;

        ++regionCount;
    }

    heapSize += size;
    mutexUnlock(&mutex);
    return (1);
}

static void *tmpMemAlloc(size_t size, u32 alignment)
{
    static void *nextPlacement = (void *) PLACEMENT_BEGIN;

    void *currPlacement = (void *) alignUp((u32) nextPlacement, alignment);
    if ((u32) currPlacement + size > PLACEMENT_END)
        return (0);

    mutexLock(&mutex);
    nextPlacement = currPlacement + size;
    mutexUnlock(&mutex);

    return (currPlacement);
}

void *kmalloc(size_t size, u32 alignment, const char *comment)
{
    static u32 consecutiveNumber = 0;

    size_t within = 0xFFFFFFFF;
    if (alignment & HEAP_WITHIN_PAGE) {
        klog("[kmalloc] invalid alignment (set pagesize)\n");
        within = PAGESIZE;
    } else if (alignment & HEAP_WITHIN_64K) {
        klog("[kmalloc] alignment heap within 64k warning\n");
        within = 0x10000;
    }
    char continuous = alignment & HEAP_CONTINUOUS;

    alignment &= HEAP_ALIGNMENT_MASK;
    size = alignUp(size, 4);

    // Use tmp malloc until paging init
    if (regions == 0) {
        return (tmpMemAlloc(size, alignment));
    }

    mutexLock(&mutex);
    char foundFree = 0;
    u8 *regionAddress = firstFreeAddr;
    for (u32 i = firstFreeRegion; i < regionCount; i++) {
        if (!regions[i].reserved)
            foundFree = 1;
        else if (!foundFree) {
            firstFreeRegion = i;
            firstFreeAddr = regionAddress;
        }

        u8 *alignedAddress = (u8 *) alignUp((u32) regionAddress, alignment);
        u32 additionalSize = (u32) alignedAddress - (u32) regionAddress;

        if (!regions[i].reserved && (regions[i].size >= size + additionalSize) &&
            (within - (u32) regionAddress % within >= additionalSize)) {
            if (continuous) {
                char iscontinuous = 1;
                for (void *virt1 = (void *) alignDown((u32) alignedAddress, PAGESIZE);
                     (u32) (virt1 + PAGESIZE) <= (u32) (alignedAddress + size); virt1 += PAGESIZE) {
                    u32 phys1 = pagingGetPhysAddr(virt1);
                    u32 phys2 = pagingGetPhysAddr(virt1 + PAGESIZE);
                    if (phys1 + PAGESIZE != phys2) {
                        iscontinuous = 0;
                        break;
                    }
                }
                if (!iscontinuous)
                    continue;
            }

            if (alignedAddress != regionAddress) {
                if (regionCount >= regionMaxCount) {
                    mutexUnlock(&mutex);
                    return (0);
                }

                memmove(regions + i + 1, regions + i, (regionCount - i) * sizeof(region_t));

                ++regionCount;

                regions[i].size = alignedAddress - regionAddress;
                regions[i].reserved = 0;

                regions[i + 1].size -= regions[i].size;

                regionAddress += regions[i].size;
                i++;
            }

            if (regions[i].size > size + additionalSize) {
                if (regionCount + 1 > regionMaxCount) {
                    mutexUnlock(&mutex);
                    return (0);
                }

                memmove(regions + i + 2, regions + i + 1, (regionCount - i - 1) * sizeof(region_t));

                ++regionCount;

                regions[i + 1].size = regions[i].size - size;
                regions[i + 1].reserved = 0;
                regions[i + 1].number = 0;

                regions[i].size = size;
            }

            regions[i].reserved = 1;
            strncpy(regions[i].comment, comment, 20);
            regions[i].comment[20] = 0;
            regions[i].number = ++consecutiveNumber;

            mutexUnlock(&mutex);
            return (regionAddress);

        }

        regionAddress += regions[i].size;
    }

    u32 sizeToGrow = MAX(HEAP_MIN_GROWTH, alignUp(size * 3 / 2, PAGESIZE));
    char success = allocHeapGrow(sizeToGrow, (u8 *) ((u32) heapStart + heapSize));

    mutexUnlock(&mutex);

    if (!success) {
        klog("[kmalloc] failed: %s, heap could not be expanded!\n", comment);
        return (0);
    }

    return kmalloc(size, alignment, comment);
}


void kfree(void *addr)
{

    if (addr == 0)
        return;

    mutexLock(&mutex);

    u8 *regionAddress = heapStart;
    for (u32 i = 0; i < regionCount; i++) {
        if (regionAddress == addr && regions[i].reserved) {
            regions[i].number = 0;
            regions[i].reserved = 0;

            if ((i + 1 < regionCount) && !regions[i + 1].reserved) {
                regions[i].size += regions[i + 1].size;
                memmove(regions + i + 1, regions + i + 2, (regionCount - 2 - i) * sizeof(region_t));
                --regionCount;
            }

            if (i > 0 && !regions[i - 1].reserved) {
                regions[i - 1].size += regions[i].size;
                memmove(regions + i, regions + i + 1, (regionCount - 1 - i) * sizeof(region_t));
                --regionCount;
            }

            if (i < firstFreeRegion) {
                firstFreeRegion = i;
                firstFreeAddr = regionAddress;
            }

            mutexUnlock(&mutex);
            return;
        }

        regionAddress += regions[i].size;
    }

    mutexUnlock(&mutex);

    klog("[kfree] Broken free: %Xh\n", addr);
}

void kmallocGetInfo(u32 *total, u32 *used)
{
    *total = 0;
    *used = 0;

    mutexLock(&mutex);
    for (u32 i = 0; i < regionCount; i++) {
        if (regions[i].reserved)
            *used += regions[i].size;

        *total += regions[i].size;
    }
    mutexUnlock(&mutex);
}