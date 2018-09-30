//
// Created by rebut_p on 30/09/18.
//

#ifndef KERNEL_EPITA_PAGING2_H
#define KERNEL_EPITA_PAGING2_H

#include "../multiboot.h"

#include <k/types.h>

#define PAGESIZE   4096
#define NBPAGE 1024
#define NBTABLE 1024

#define FOUR_GB 0x100000000ull
#define PLACEMENT_BEGIN 0x600000
#define PLACEMENT_END 0xC00000
#define KERNEL_HEAP_START 0xE0000000

struct TableDirectory {
    u32 pages[NBPAGE];
} __attribute__((packed));

struct PageDirectory {
    u32 tables[NBTABLE];
    struct TableDirectory *tablesAddr[NBTABLE];
    u32 physAddr;
} __attribute__((packed));

enum MEMFLAGS {
    MEM_PRESENT = 1,
    MEM_WRITE = 1 << 1,
    MEM_USER = 1 << 2,
    MEM_WRITETHROUGH = 1 << 3,
    MEM_NOCACHE = 1 << 4,
    MEM_NOTLBUPDATE = 1 << 8,
    MEM_ALLOCATED = 1 << 9
};

u32 initPaging(memory_map_t *memoryMap, u32 size);
void switchPaging(struct PageDirectory *pd);

struct PageDirectory *createPageDirrectory();
void destroyPageDirectory(struct PageDirectory *pd);

int paging_alloc(struct PageDirectory *pd, void *addr, u32 size, enum MEMFLAGS flags);
void paging_free(struct PageDirectory *pd, void *virtAddress, u32 size);

u32 getPhysAddr(const void *vaddr);

extern struct PageDirectory *kernelPageDirectory;
extern struct PageDirectory *currentPageDirectory;

#endif //KERNEL_EPITA_PAGING2_H
