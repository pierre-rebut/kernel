//
// Created by rebut_p on 15/11/18.
//

#ifndef KERNEL_EPITA_PHYSICAL_MEMORY_H
#define KERNEL_EPITA_PHYSICAL_MEMORY_H

#include <stddef.h>
#include <multiboot.h>

#define MAX_MEMORY 0x100000000ull
#define PAGESIZE 4096

static inline u32 alignUp(u32 val, u32 alignment)
{
    if (!alignment)
        return val;
    --alignment;
    return (val + alignment) & ~alignment;
}

static inline u32 alignDown(u32 val, u32 alignment)
{
    if (!alignment)
        return val;
    return val & ~(alignment - 1);
}

u32 initPhysicalMemory(const multiboot_info_t *info);

u32 allocPhysicalMemory();

void freePhysicalMemory(u32 alloc);

u32 physmemGetTotalPages();

u32 getTotalPhysMemory();

u32 getTotalUsedPhysMemory();

#endif //KERNEL_EPITA_PHYSICAL_MEMORY_H
