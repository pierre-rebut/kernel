//
// Created by rebut_p on 15/11/18.
//

#include <string.h>
#include <stdio.h>
#include <include/multiboot.h>

#include "physical-memory.h"
#include "allocator.h"

static u32 *physicalMemTable = NULL;
static u32 physicalMemSize;

static u32 getMemorySize(memory_map_t *memEntry, memory_map_t *memEnd) {
    u32 memSize = 0;

    while (memEntry < memEnd) {

        if (memEntry->regionAddr < MAX_MEMORY && memEntry->regionSize != 0) {
            if (memEntry->regionAddr + memEntry->regionSize >= MAX_MEMORY)
                memEntry->regionSize = ((u32) MAX_MEMORY - memEntry->regionAddr);

            memSize = MAX(memSize, (u32) (memEntry->regionAddr + memEntry->regionSize));
        }
        memEntry = (memory_map_t *) ((void *) memEntry + memEntry->entrySize + 4);
    }

    return memSize;
}

static void memorySetRegion(u32 addrBegin, u32 addrEnd, u8 isReserved) {
    u32 start = alignUp(addrBegin, PAGESIZE) / PAGESIZE;
    u32 end = alignDown(addrEnd, PAGESIZE) / PAGESIZE;

    for (u32 i = start; i < end; i++) {
        u32 val = (u32) 1 << (i % 32);
        if (isReserved)
            physicalMemTable[i / 32] |= val;
        else
            physicalMemTable[i / 32] &= ~val;
    }
}

u32 initPhysicalMemory(const multiboot_info_t *info) {
    memory_map_t *memEntry = (memory_map_t*)info->mmap_addr;
    memory_map_t *memEnd = ((void *) memEntry + info->mmap_length);

    u32 memSize = getMemorySize(memEntry, memEnd);
    physicalMemSize = memSize / PAGESIZE / 32;

    physicalMemTable = kmalloc(physicalMemSize * 4, 0, "physicalMemTable");
    memset(physicalMemTable, 0xFF, physicalMemSize * 4);

    while (memEntry < memEnd) {
        if (memEntry->type == 1 && memEntry->regionAddr < MAX_MEMORY) {
            memorySetRegion((u32) memEntry->regionAddr, (u32) (memEntry->regionAddr + memEntry->regionSize), 0);
        }
        memEntry = (memory_map_t *) ((void *) memEntry + memEntry->entrySize + 4);
    }

    memorySetRegion(0x00, PLACEMENT_END, 1);
    return memSize;
}

u32 allocPhysicalMemory() {

    for (u32 i = 0; i < physicalMemSize; i++) {
        if (physicalMemTable[i] == 0xFFFFFFFF)
            continue;

        u32 freeBite = 0;
        asm volatile ("bsfl %1, %0\n\t" : "=r"(freeBite) : "r"(~physicalMemTable[i]));

        physicalMemTable[i] |= 1 << freeBite;
        return (i * 32 + freeBite) * PAGESIZE;
    }
    return 0;
}

void freePhysicalMemory(u32 allocAddr) {
    u32 i = allocAddr / PAGESIZE;
    physicalMemTable[i] &= ~(1 << i / 32);
}