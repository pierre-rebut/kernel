#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "paging.h"
#include "physical-memory.h"

static region_t *regions = 0;
static u32 regionCount = 0;
static u32 regionMaxCount = 0;
static u32 firstFreeRegion = 0;
static void *firstFreeAddr = (void *) KERNEL_HEAP_START;
static u8 const *heapStart = (void *) KERNEL_HEAP_START;
static u32 heapSize = 0;
static const u32 HEAP_MIN_GROWTH = 0x10000;

static void *placementMalloc(size_t size, u32 alignment);

int initAllocator(void) {
    regions = placementMalloc(0, 0);

    regionCount = 0;
    regionMaxCount = (PLACEMENT_END - (u32) regions) / sizeof(region_t);
    return 0;
}

void *heap_getCurrentEnd(void) {
    return (heapStart + heapSize);
}

static char heap_grow(size_t size, u8 *heapEnd) {
    if ((regionCount > 0) && regions[regionCount - 1].reserved && (regionCount >= regionMaxCount))
        return 0;

    if (!pagingAlloc(kernelPageDirectory, heapEnd, size, MEM_WRITE))
        return 0;

    if ((regionCount > 0) && !regions[regionCount - 1].reserved)
        regions[regionCount - 1].size += size;
    else {
        regions[regionCount].reserved = 0;
        regions[regionCount].size = size;
        regions[regionCount].number = 0;

        ++regionCount;
    }

    heapSize += size;
    return 1;
}

static void *placementMalloc(size_t size, u32 alignment) {
    static void *nextPlacement = (void *) PLACEMENT_BEGIN;

    size = alignUp(size, 4);

    void *currPlacement = (void *) alignUp((u32) nextPlacement, alignment);

    if ((u32) currPlacement + size > PLACEMENT_END)
        return (0);

    nextPlacement = currPlacement + size;
    return currPlacement;
}

void *kmalloc(size_t size, u32 alignment) {
    static u32 consecutiveNumber = 0;

    size_t within = 0xFFFFFFFF;
    if (alignment & HEAP_WITHIN_PAGE) {
        within = PAGESIZE;
    } else if (alignment & HEAP_WITHIN_64K) {
        within = 0x10000;
    }

    char continuous = alignment & HEAP_CONTINUOUS;
    alignment &= HEAP_ALIGNMENT_MASK;

    if (regions == 0) {
        return (placementMalloc(size, alignment));
    }

    size = alignUp(size, 4);

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
                     (u32)(virt1 + PAGESIZE) <= (u32)(alignedAddress + size); virt1 += PAGESIZE) {
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
                if (regionCount >= regionMaxCount)
                    return 0;

                memcpy(regions + i + 1, regions + i, (regionCount - i) * sizeof(region_t));

                ++regionCount;

                regions[i].size = alignedAddress - regionAddress;
                regions[i].reserved = 0;

                regions[i + 1].size -= regions[i].size;

                regionAddress += regions[i].size;
                i++;
            }

            // Split the leftover
            if (regions[i].size > size + additionalSize) {
                if (regionCount + 1 > regionMaxCount)
                    return 0;

                memcpy(regions + i + 2, regions + i + 1, (regionCount - i - 1) * sizeof(region_t));

                ++regionCount;

                // Setup the regions
                regions[i + 1].size = regions[i].size - size;
                regions[i + 1].reserved = 0;
                regions[i + 1].number = 0;

                regions[i].size = size;
            }

            regions[i].reserved = 1;
            regions[i].number = ++consecutiveNumber;

            return (regionAddress);

        }

        regionAddress += regions[i].size;
    }

    u32 sizeToGrow = MAX(HEAP_MIN_GROWTH, alignUp(size * 3 / 2, PAGESIZE));
    char success = heap_grow(sizeToGrow, (u8*)((u32) heapStart + heapSize));

    if (!success) {
        kSerialPrintf("\nmalloc (\"%s\") failed, heap could not be expanded!");
        return (0);
    }

    return kmalloc(size, alignment);
}


void kfree(void *addr) {
    if (addr == 0)
        return;

    u8 *regionAddress = heapStart;
    for (u32 i = 0; i < regionCount; i++) {
        if (regionAddress == addr && regions[i].reserved) {

            regions[i].number = 0;
            regions[i].reserved = 0;

            if ((i + 1 < regionCount) && !regions[i + 1].reserved) {
                regions[i].size += regions[i + 1].size;

                memcpy(regions + i + 1, regions + i + 2, (regionCount - 2 - i) * sizeof(region_t));

                --regionCount;
            }

            if (i > 0 && !regions[i - 1].reserved) {
                regions[i - 1].size += regions[i].size; // merge

                memcpy(regions + i, regions + i + 1, (regionCount - 1 - i) * sizeof(region_t));

                --regionCount;
            }

            if (i < firstFreeRegion) {
                firstFreeRegion = i;
                firstFreeAddr = regionAddress;
            }

            return;
        }

        regionAddress += regions[i].size;
    }

    kSerialPrintf("\nBroken free: %Xh", addr);
}


