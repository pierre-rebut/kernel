//
// Created by rebut_p on 16/11/18.
//

#ifndef KERNEL_EPITA_PAGGING_H
#define KERNEL_EPITA_PAGGING_H

#include <k/types.h>

#define NB_TABLE 1024
#define NB_PAGE 1024

struct TableDirectory {
    u32 pages[NB_PAGE];
};

struct PageDirectory {
    u32 tablesAddr[NB_TABLE];
    struct TableDirectory *tablesInfo[NB_TABLE];
    u32 physAddr;
};

enum MEMFLAGS {
    MEM_PRESENT = 1,
    MEM_WRITE = 1 << 1,
    MEM_USER = 1 << 2,
    MEM_WRITETHROUGH = 1 << 3,
    MEM_NOCACHE = 1 << 4,
    MEM_NOTLBUPDATE = 1 << 8,
    MEM_ALLOCATED = 1 << 9
};

void initPaging(u32 memSize);
struct PageDirectory *createPageDirectory();
void destroyPageDirectory(struct PageDirectory *pageDirectory);
void switchPageDirectory(struct PageDirectory *pageDirectory);

int pagingAlloc(struct PageDirectory *pd, void *addr, u32 size, enum MEMFLAGS flags);
void pagingFree(struct PageDirectory *pd, void *virtAddress, u32 size);
int pagingSetFlags(struct PageDirectory *pd, void *virtAddress, u32 size, enum MEMFLAGS flags);

extern struct PageDirectory *kernelPageDirectory;
extern struct PageDirectory *currentPageDirectory;

#endif //KERNEL_EPITA_PAGGING_H
