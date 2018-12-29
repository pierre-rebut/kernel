
#include "allocator.h"
#include "paging.h"
#include "physical-memory.h"
#include <stdio.h>
#include <string.h>

/* The heap provides the malloc/free-functionality, i.e. dynamic allocation of memory.
   It manages a certain amount of continuous virtual memory, starting at "heapStart".
   Whenever more memory is requested than there is available, the heap expands.
   For expansion, the heap asks the paging module to map physical memory to the following virtual addresses
   and increases its "heapSize" variable (but at least by "HEAP_MIN_GROWTH") afterwards.

   To manage the free and reserved (allocated) areas of the heap an array of "region" elements is kept.
   Each region specifies its size and reservation status.
   Free regions always get merged. Regions don't store their addresses.
   The third region address is calculated by adding the first and second region size to "heapStart":
   region_3_addr = heapStart + regions[0].size + regions[1].size

   Before the heap is set up, memory is allocated on a "placement address".
   This is an identity mapped area of continuous memory,
   the allocation just moves a pointer forward by the requested size and returns its previous value.

   The heap's management data is placed at this placement address, too.
   Since this area cannot grow, the heap has a maximum amount of region objects ("regionMaxCount").*/

// TODO: Ensure the heap will not overflow (above KERNEL_heapEnd, cf. memory.h)


typedef struct {
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

#ifdef _MEMLEAK_FIND_
static u32 counter = 0;
#endif


static void *placementMalloc(size_t size, u32 alignment);


int initAllocator(void) {
    // This gets us the current placement address
    regions = placementMalloc(0, 0);

    // We take the rest of the placement area
    regionCount = 0;
    regionMaxCount = (PLACEMENT_END - (u32) regions) / sizeof(region_t);
    return 0;
}

void *heap_getCurrentEnd(void) {
    return (heapStart + heapSize);
}

static char heap_grow(size_t size, u8 *heapEnd, char continuous) {
    // We will have to append another region-object to our array if we can't merge with the last region - check whether there would be enough space to insert the region-object
    if ((regionCount > 0) && regions[regionCount - 1].reserved && (regionCount >= regionMaxCount)) {
        return (0);
    }

    mutexLock(&mutex);
    // Enhance the memory
    if (pagingAlloc(kernelPageDirectory, heapEnd, size, MEM_WRITE)) {
        mutexUnlock(&mutex);
        return (0);
    }

    // Maybe we can merge with the last region object?
    if ((regionCount > 0) && !regions[regionCount - 1].reserved) {
        regions[regionCount - 1].size += size;
    }
        // Otherwise insert a new region object
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

static void *placementMalloc(size_t size, u32 alignment) {
    static void *nextPlacement = (void *) PLACEMENT_BEGIN;

    // Avoid odd addresses
    size = alignUp(size, 4);

    // Ensure alignment
    void *currPlacement = (void *) alignUp((u32) nextPlacement, alignment);

    // Check if there is enough space in placement area
    if ((u32) currPlacement + size > PLACEMENT_END)
        return (0);

    // Do simple placement allocation
    mutexLock(&mutex);
    nextPlacement = currPlacement + size;
    mutexUnlock(&mutex);

    return (currPlacement);
}

void *kmalloc(size_t size, u32 alignment, const char *comment) {
    // consecutive number for detecting the sequence of mallocs at the heap
    static u32 consecutiveNumber = 0;

    // Analyze alignment and other requirements
    size_t within = 0xFFFFFFFF;
    if (alignment & HEAP_WITHIN_PAGE) {
        kSerialPrintf("kmalloc alignment warning\n");
        //ASSERT(size <= PAGESIZE);
        within = PAGESIZE;
    } else if (alignment & HEAP_WITHIN_64K) {
        kSerialPrintf("kmalloc alignment heap within 64k warning\n");
        //ASSERT(size <= 0x10000);
        within = 0x10000;
    }
    char continuous = alignment & HEAP_CONTINUOUS;

    alignment &= HEAP_ALIGNMENT_MASK;

    // If the heap is not set up, do placement malloc
    if (regions == 0) {
        return (placementMalloc(size, alignment));
    }

    // Avoid odd addresses
    size = alignUp(size, 4);

    mutexLock(&mutex);
    // Walk the regions and find one being suitable
    char foundFree = 0;
    u8 *regionAddress = firstFreeAddr;
    for (u32 i = firstFreeRegion; i < regionCount; i++) {
        // Manage caching of first free region
        if (!regions[i].reserved)
            foundFree = 1;
        else if (!foundFree) {
            firstFreeRegion = i;
            firstFreeAddr = regionAddress;
        }

        // Calculate aligned address and the additional size needed due to alignment
        u8 *alignedAddress = (u8 *) alignUp((u32) regionAddress, alignment);
        u32 additionalSize = (u32) alignedAddress - (u32) regionAddress;

        // Check whether this region is free, big enough and fits page requirements
        if (!regions[i].reserved && (regions[i].size >= size + additionalSize) &&
            (within - (u32) regionAddress % within >= additionalSize)) {
            // Check if the region consists of continuous physical memory if required
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

            // We will split up this region ...
            // +--------------------------------------------------------+
            // |                      Current Region                    |
            // +--------------------------------------------------------+
            //
            // ... into three, the first and third remain free,
            // while the second gets reserved, and its address is returned
            //
            // +------------------+--------------------------+----------+
            // | Before Alignment | Aligned Destination Area | Leftover |
            // +------------------+--------------------------+----------+

            // Split the pre-alignment area
            if (alignedAddress != regionAddress) {
                // Check whether we are able to expand
                if (regionCount >= regionMaxCount) {
                    mutexUnlock(&mutex);
                    return (0);
                }

                // Move all following regions ahead to get room for a new one
                memmove(regions + i + 1, regions + i, (regionCount - i) * sizeof(region_t));

                ++regionCount;

                // Setup the regions
                regions[i].size = alignedAddress - regionAddress;
                regions[i].reserved = 0;

                regions[i + 1].size -= regions[i].size;

                // "Aligned Destination Area" becomes the "current" region
                regionAddress += regions[i].size;
                i++;
            }

            // Split the leftover
            if (regions[i].size > size + additionalSize) {
                // Check whether we are able to expand
                if (regionCount + 1 > regionMaxCount) {
                    mutexUnlock(&mutex);
                    return (0);
                }

                // Move all following regions ahead to get room for a new one
                memmove(regions + i + 2, regions + i + 1, (regionCount - i - 1) * sizeof(region_t));

                ++regionCount;

                // Setup the regions
                regions[i + 1].size = regions[i].size - size;
                regions[i + 1].reserved = 0;
                regions[i + 1].number = 0;

                regions[i].size = size;
            }

            // Set the region to "reserved" and return its address
            regions[i].reserved = 1;
            strncpy(regions[i].comment, comment, 20);
            regions[i].comment[20] = 0;
            regions[i].number = ++consecutiveNumber;

#ifdef _MEMLEAK_FIND_
            counter++;
            writeInfo(2, "Malloc - free: %u", counter);
#endif
#ifdef _MALLOC_FREE_LOG_
            textColor(YELLOW);
            printf("\nmalloc: %Xh %s", regionAddress, comment);
            textColor(TEXT);
#endif

            mutexUnlock(&mutex);
            return (regionAddress);

        } //region is free and big enough

        regionAddress += regions[i].size;
    }

    // There is nothing free, try to expand the heap
    u32 sizeToGrow = MAX(HEAP_MIN_GROWTH, alignUp(size * 3 / 2, PAGESIZE));
    char success = heap_grow(sizeToGrow, (u8 *) ((u32) heapStart + heapSize), continuous);

    mutexUnlock(&mutex);

    if (!success) {
        kSerialPrintf("\nmalloc (\"%s\") failed, heap could not be expanded!", comment);
        return (0);
    } else {
#ifdef _MALLOC_FREE_LOG_
        textColor(YELLOW);
        printf("\nheap expanded: %Xh heap end: %Xh", sizeToGrow, (u32)heapStart + heapSize);
        textColor(TEXT);
#endif
    }

    // Now there should be a region that is large enough
    return kmalloc(size, alignment, comment);
}


void kfree(void *addr) {
#ifdef _MALLOC_FREE_LOG_
    textColor(LIGHT_GRAY);
    printf("\nfree:   %Xh", addr);
    textColor(TEXT);
#endif

    if (addr == 0) {
        return;
    }

#ifdef _MEMLEAK_FIND_
    counter--;
    writeInfo(2, "Malloc - free: %u", counter);
#endif

    mutexLock(&mutex);

    // Walk the regions and find the correct one
    u8 *regionAddress = heapStart;
    for (u32 i = 0; i < regionCount; i++) {
        if (regionAddress == addr && regions[i].reserved) {
#ifdef _MALLOC_FREE_LOG_
            textColor(LIGHT_GRAY);
            printf(" %s", regions[i].comment);
            textColor(TEXT);
#endif
            regions[i].number = 0;
            regions[i].reserved = 0; // free the region

            // Check for a merge with the next region
            if ((i + 1 < regionCount) && !regions[i + 1].reserved) {
                // Adjust the size of the now free region
                regions[i].size += regions[i + 1].size; // merge

                // Move all following regions back by one
                memmove(regions + i + 1, regions + i + 2, (regionCount - 2 - i) * sizeof(region_t));

                --regionCount;
            }

            // Check for a merge with the previous region
            if (i > 0 && !regions[i - 1].reserved) {
                // Adjust the size of the previous region
                regions[i - 1].size += regions[i].size; // merge

                // Move all following regions back by one
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

    kSerialPrintf("\nBroken free: %Xh", addr);
}

void heap_logRegions(void) {
    kSerialPrintf("\nDebug: Heap regions sent to serial output.\n");
    kSerialPrintf("\n\nregionMaxCount: %u, regionCount: %u, firstFreeRegion: %u\n", regionMaxCount, regionCount,
                  firstFreeRegion);
    kSerialPrintf("\n---------------- HEAP REGIONS ----------------\n");
    kSerialPrintf("#\taddress\t\tsize\t\tnumber\tcomment");

    u32 regionAddress = (u32) heapStart;

    for (u32 i = 0; i < regionCount; i++) {
        if (regions[i].reserved)
            kSerialPrintf("\n%u\t%Xh\t%Xh\t%u\t%s", i, regionAddress, regions[i].size, regions[i].number,
                          regions[i].comment);
        else
            kSerialPrintf("\n%u\t%Xh\t%Xh\t-\t-", i, regionAddress, regions[i].size);
        regionAddress += regions[i].size;
    }
    kSerialPrintf("\n---------------- HEAP REGIONS ----------------\n\n");
}