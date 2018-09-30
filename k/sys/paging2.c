//
// Created by rebut_p on 30/09/18.
//

#include "paging2.h"
#include "kalloc.h"

#include <stdio.h>
#include <utils.h>
#include <string.h>

static u32 framesResaSize = 0;
static u32 *framesResaTable;

struct PageDirectory *kernelPageDirectory;
struct PageDirectory *currentPageDirectory = NULL;

static u32 initMemory(memory_map_t *memoryMap, size_t memoryMapLength);

u32 initPaging(memory_map_t *memoryMap, u32 memoryMapLength) {
    u32 totalRam = initMemory(memoryMap, memoryMapLength);

    kernelPageDirectory = kmalloc(sizeof(struct PageDirectory), PAGESIZE);
    memset(kernelPageDirectory, 0, sizeof(struct PageDirectory) - 4);
    kernelPageDirectory->physAddr = (u32) kernelPageDirectory;

    u32 addr = 0;
    for (u8 i = 0; i < 3; i++) {
        kernelPageDirectory->tablesAddr[i] = kmalloc(sizeof(struct TableDirectory), PAGESIZE);
        kernelPageDirectory->tables[i] = (u32) kernelPageDirectory->tablesAddr[i] | MEM_PRESENT | MEM_WRITE;

        for (u32 j = 0; j < NBPAGE; j++) {
            u32 flags = MEM_PRESENT | MEM_WRITE | MEM_ALLOCATED;
            if (addr >= 0x100000)
                flags |= MEM_NOTLBUPDATE;

            kernelPageDirectory->tablesAddr[i]->pages[j] = addr | flags;
            addr += PAGESIZE;
        }
    }

    kernelPageDirectory->tablesAddr[0]->pages[0] = 0 | MEM_PRESENT | MEM_ALLOCATED;

    size_t kernelpts = totalRam / PAGESIZE / NBPAGE;
    struct TableDirectory *heap_pts = kmalloc(kernelpts * sizeof(struct TableDirectory), PAGESIZE);
    memset(heap_pts, 0, kernelpts * sizeof(struct TableDirectory));

    for (u32 i = 0; i < kernelpts; i++) {
        kernelPageDirectory->tablesAddr[KERNEL_HEAP_START / PAGESIZE / NBPAGE + i] = heap_pts + i;
        kernelPageDirectory->tables[KERNEL_HEAP_START / PAGESIZE / NBPAGE + i] = (u32) (heap_pts + i) | MEM_PRESENT;
    }

    switchPaging(kernelPageDirectory);

    u32 cr0;
    asm volatile ("movl %%cr0, %0": "=r"(cr0));
    cr0 |= (1 << 31) | (1 << 16);
    asm volatile ("movl %0, %%cr0": :"a"(cr0));

    return totalRam;
}

void switchPaging(struct PageDirectory *pd) {
    if (pd != currentPageDirectory) {
        printf("\nDEBUG: paging_switch: pd=%X, pd->physAddr=%X\n", (u32) pd, pd->physAddr);

        currentPageDirectory = pd;
        asm volatile("mov %0, %%cr3" : : "r"(pd->physAddr));
    }
}

static u32 getMemorySize(memory_map_t *memoryMap, memory_map_t *memoryMapEnd) {
    u32 size = 0;

    for (memory_map_t *entry = memoryMap; entry < memoryMapEnd;) {
        if (entry->regionBase < FOUR_GB && entry->regionSize != 0) {
            if (entry->regionBase + entry->regionSize >= FOUR_GB)
                entry->regionSize = ((u32) FOUR_GB - entry->regionBase);

            size = max(size, (u32) (entry->regionBase + entry->regionSize));
        }

        entry = (memory_map_t *) ((void *) entry + entry->entrySize + 4);
    }
    return size;
}

static void memorySetRegion(u32 addrBegin, u32 addrEnd, int reserved) {
    u32 start = alignUp(addrBegin, PAGESIZE) / PAGESIZE;
    u32 end = alignDown(addrEnd, PAGESIZE) / PAGESIZE;

    for (u32 i = start; i < end; i++) {
        u32 val = (u32) 1 << (i % 32);
        if (reserved)
            framesResaTable[i / 32] |= val;
        else
            framesResaTable[i / 32] &= ~val;
    }
}

static u32 initMemory(memory_map_t *memoryMap, size_t memoryMapLength) {
    memory_map_t *memoryMapEnd = (memory_map_t *) ((void *) memoryMap + memoryMapLength);
    printf("Memory map (%X -> %X):\n", (u32) memoryMap, (u32) memoryMapEnd);

    u32 memSize = getMemorySize(memoryMap, memoryMapEnd);
    framesResaSize = memSize / PAGESIZE / 32;

    framesResaTable = kmalloc(framesResaSize * 4, 0);
    memset(framesResaTable, 0xFF, framesResaSize * 4);

    for (memory_map_t *entry = memoryMap; entry < memoryMapEnd;) {
        if (entry->type == 1 && entry->regionBase < FOUR_GB)
            memorySetRegion(entry->regionBase, entry->regionBase + entry->regionSize, 0);
        entry = (memory_map_t *) ((void *) entry + entry->entrySize + 4);
    }

    return memSize;
}

static inline void invlpg(void *p) {
    asm volatile("invlpg (%0)"::"r" (p) : "memory");
}

u32 allocFrame() {
    for (u32 i = PLACEMENT_END / PAGESIZE / 32; i < framesResaSize; i++) {
        if (framesResaTable[i] == 0xFFFFFFFF)
            continue;

        u32 freeBite = 0;
        asm volatile ("bsfl %1, %0" : "=r"(freeBite) : "r"(~framesResaTable[i])); // Get first free bit (faster than c)

        framesResaTable[i] |= 1 << freeBite;
        return (i * 32 + freeBite) * PAGESIZE;
    }
    return 0;
}

void freeFrame(u32 addr) {
    u32 i = addr / PAGESIZE;
    framesResaTable[i] &= ~(1 << i / 32);
}

u32 allocFrames(u32 n, u32 *frames) {
    for (u32 i = 0; i < n; i++) {
        frames[i] = allocFrame();
        if (frames[i] == 0)
            return i;
    }
    return n;
}

void freeFrames(u32 n, u32 *frames) {
    for (u32 i = 0; i < n; i++)
        freeFrame(frames[i]);
}

void freePages(struct PageDirectory *pd, void *addr, size_t pages) {
    for (u32 i = 0; i < pages; i++) {
        u32 pageNumber = (u32) addr / PAGESIZE + i;

        if (!pd->tablesAddr[pageNumber / NBTABLE])
            return;

        pd->tablesAddr[pageNumber / NBTABLE]->pages[pageNumber % NBPAGE] = 0;

        if (pd->tablesAddr[pageNumber / NBTABLE] == currentPageDirectory->tablesAddr[pageNumber / NBTABLE])
            invlpg((void *) (pageNumber * PAGESIZE));
    }
}

u32 getPhysAddr(const void *vaddr) {
    u32 pageNumber = (u32) vaddr / PAGESIZE;
    struct TableDirectory *pt = currentPageDirectory->tablesAddr[pageNumber / NBTABLE];
    if (!pt)
        return 0;

    printf("\nvirt-->phys: pagenr: %u, pt: %Xh\n", pageNumber, (u32) pt);
    return ((pt->pages[pageNumber % NBPAGE] & 0xFFFFF000) + ((u32) vaddr & 0x00000FFF));
}

int allocPages(struct PageDirectory *pd, void *addr, size_t pages) {
    for (u32 i = 0; i < pages; i++) {
        u32 pageNumber = (u32) addr / PAGESIZE + i;

        struct TableDirectory *pt = pd->tablesAddr[pageNumber / NBTABLE];
        if (!pt) {
            pt = kmalloc(sizeof(struct TableDirectory), PAGESIZE);
            if (!pt) {
                freePages(pd, addr, i * PAGESIZE);
                return 0;
            }

            memset(pt, 0, sizeof(struct TableDirectory));
            pd->tablesAddr[pageNumber / NBTABLE] = pt;
            pd->tables[pageNumber / NBTABLE] = getPhysAddr(pt) | MEM_PRESENT | MEM_WRITE;
        } else {
            if (pt->pages[pageNumber % NBPAGE] & MEM_ALLOCATED) {
                printf("Page already allocated: %u\n", pageNumber);
                freePages(pd, addr, i * PAGESIZE);
                return 0;
            }
        }

        pt->pages[pageNumber % NBPAGE] = MEM_ALLOCATED;
    }
    return 1;
}

int mapPagesToFrames(struct PageDirectory *pd, void *vaddr, u32 paddr, size_t pages, enum MEMFLAGS flags) {
    for (u32 i = 0; i < pages; i++) {
        u32 pageNumber = (u32) vaddr / PAGESIZE + i;
        struct TableDirectory *pt = pd->tablesAddr[pageNumber / NBTABLE];

        if (!pt || !(pt->pages[pageNumber % NBPAGE] & MEM_ALLOCATED)) {
            printf("Page not allocated: %u\n", pageNumber);
            return 0;
        }

        pd->tables[pageNumber / NBTABLE] = getPhysAddr(pt) | MEM_PRESENT | MEM_WRITE;
        pd->tables[pageNumber / NBTABLE] |= (flags & (~MEM_NOTLBUPDATE)); // Update codes

        pt->pages[pageNumber % NBPAGE] = (paddr + i * PAGESIZE) | flags | MEM_PRESENT | MEM_ALLOCATED;

        if (pt == currentPageDirectory->tablesAddr[pageNumber / NBTABLE])
            invlpg(vaddr + i * PAGESIZE);

        if (flags & MEM_USER)
            printf("page %u now associated to physAddress %Xh\n", pageNumber, paddr + i * PAGESIZE);
    }
    return 1;
}

int paging_alloc(struct PageDirectory *pd, void *addr, u32 size, enum MEMFLAGS flags) {
    // "virtAddress" and "size" must be page-aligned
    // ASSERT(((u32) addr) % PAGESIZE == 0);
    if (size % PAGESIZE != 0) {
        printf("size not page aligned\n");
        return -1;
    }

    u32 pages = size / PAGESIZE;

    if (!allocPages(pd, addr, pages)) {
        printf("allocPages(%X, %X, %u) failed.\n", (u32) pd, (u32) addr, (u32) pages);
        return -2;
    }

    u32 *lstFrames = kmalloc(sizeof(u32) * pages, 0);
    u32 tmp = allocFrames(pages, lstFrames);
    if (tmp != pages) {
        freeFrames(tmp, lstFrames);
        kfree(lstFrames);
        printf("allocFrames err (expected %d, res %d)\n", (u32) pages, tmp);
        return -3;
    }

    for (u32 i = 0; i < pages; i++) {
        if (!mapPagesToFrames(pd, addr + i * PAGESIZE, lstFrames[i], 1, flags)) {
            freeFrames(pages, lstFrames);
            kfree(lstFrames);
            printf("mapPagesToFrames(%X, %X, %X, %u, %X) failed.\n", (u32) pd, (u32) (addr + i * PAGESIZE),
                   lstFrames[i], 1, flags);
            return -4;
        }

        if (flags & MEM_USER)
            printf("pagenumber now allocated: %u physAddress: %Xh\n", i, lstFrames[i]);
    }

    kfree(lstFrames);
    return 0;
}

void paging_free(struct PageDirectory *pd, void *virtAddress, u32 size) {
    // "virtAddress" and "size" must be page-aligned
    // ASSERT(((u32) virtAddress) % PAGESIZE == 0);
    if (size % PAGESIZE != 0) {
        printf("size must be page aligned\n");
        return;
    }

    u32 pageNumber = (u32) virtAddress / PAGESIZE;
    while (size) {
        if (!pd->tablesAddr[pageNumber / NBPAGE])
            return;

        u32 physAddress = getPhysAddr(virtAddress);

        freePages(pd, virtAddress, 1);
        freeFrame(physAddress);

        pageNumber++;
        size -= PAGESIZE;
    }
}

struct PageDirectory *createPageDirrectory() {
    struct PageDirectory *pd = kmalloc(sizeof(struct PageDirectory), PAGESIZE);
    if (!pd)
        return (0);

    memcpy(pd, kernelPageDirectory, sizeof(struct PageDirectory));
    pd->physAddr = getPhysAddr(pd->tables);

    return (pd);
}

void destroyPageDirectory(struct PageDirectory *pd) {
    if (pd != kernelPageDirectory) {
        printf("can not remove kernel page directory\n");
        return;
    }

    if (pd == currentPageDirectory)
        switchPaging(kernelPageDirectory);

    for (u32 i = 0; i < NBTABLE; i++) {
        if (pd->tables[i]) {
            for (u32 j = 0; j < NBPAGE; j++) {
                u32 physAddress = pd->tablesAddr[i]->pages[j] & 0xFFFFF000;

                if (physAddress) {
                    freeFrame(physAddress);
                }
            }
            kfree(pd->tables[i]);
        }
    }
    kfree(pd);
}