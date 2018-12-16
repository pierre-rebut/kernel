//
// Created by rebut_p on 16/11/18.
//

#include <string.h>
#include <stdio.h>

#include "paging.h"
#include "allocator.h"
#include "physical-memory.h"

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
#define LOG(x, ...)

struct PageDirectory *kernelPageDirectory = NULL;
static struct PageDirectory *currentPageDirectory = NULL;

static int allocPages(struct PageDirectory *pd, void *addr, u32 pages);
static void freePages(struct PageDirectory *pd, void *addr, u32 pages);

void initPaging(u32 memSize) {

    kernelPageDirectory = kmalloc(sizeof(struct PageDirectory), PAGESIZE, "kernelPageDirectory");
    memset(kernelPageDirectory, 0, sizeof(struct PageDirectory) - 4);
    kernelPageDirectory->physAddr = (u32) kernelPageDirectory;

    u32 addr = 0;
    for (u8 i = 0; i < 3; i++) {
        kernelPageDirectory->tablesInfo[i] = kmalloc(sizeof(struct TableDirectory), PAGESIZE, "kernelTableDirectory");
        kernelPageDirectory->tablesAddr[i] = (u32) kernelPageDirectory->tablesInfo[i] | MEM_PRESENT | MEM_WRITE;

        for (u32 j = 0; j < NB_PAGE; j++) {
            u32 flags = MEM_PRESENT | MEM_WRITE | MEM_ALLOCATED;
            if (addr >= 0x100000)
                flags |= MEM_NOTLBUPDATE;

            kernelPageDirectory->tablesInfo[i]->pages[j] = addr | flags;
            addr += PAGESIZE;
        }
    }

    kernelPageDirectory->tablesInfo[0]->pages[0] = 0 | MEM_PRESENT | MEM_ALLOCATED;

    u32 kernelpts = MIN(NB_TABLE / 8, memSize / PAGESIZE / NB_PAGE);

    struct TableDirectory *heap_pts = kmalloc(kernelpts * sizeof(struct TableDirectory), PAGESIZE, "kernelHeap");
    memset(heap_pts, 0, kernelpts * sizeof(struct TableDirectory));

    for (u32 i = 0; i < kernelpts; i++) {
        kernelPageDirectory->tablesInfo[KERNEL_HEAP_START / PAGESIZE / NB_TABLE + i] = heap_pts + i;
        kernelPageDirectory->tablesAddr[KERNEL_HEAP_START / PAGESIZE / NB_TABLE + i] =
                (u32) (heap_pts + i) | MEM_PRESENT;
    }

    pagingSwitchPageDirectory(kernelPageDirectory);

    u32 cr0;
    asm volatile ("movl %%cr0, %0": "=r"(cr0));
    cr0 |= (1 << 31) | (1 << 16);
    asm volatile ("movl %0, %%cr0": :"a"(cr0));
}

static inline void invlpg(void *p) {
    LOG("invalide page\n");
    asm volatile("invlpg (%0)"::"r" (p) : "memory");
}

u32 pagingGetPhysAddr(const void *vaddr) {
    u32 pageNumber = (u32) vaddr / PAGESIZE;
    struct TableDirectory *pt = currentPageDirectory->tablesInfo[pageNumber / NB_TABLE];
    if (!pt)
        return 0;

    LOG("GetPhysAddr: virt-->phys: pageNb: %u, pt: %Xh\n", pageNumber, (u32) pt);
    return ((pt->pages[pageNumber % NB_PAGE] & 0xFFFFF000) + ((u32) vaddr & 0x00000FFF));
}

struct PageDirectory *pagingCreatePageDirectory() {
    struct PageDirectory *pageDirectory = kmalloc(sizeof(struct PageDirectory), PAGESIZE, "newPageDirectory");
    if (!pageDirectory)
        return NULL;

    memcpy(pageDirectory, kernelPageDirectory, sizeof(struct PageDirectory) - 4);
    pageDirectory->physAddr = pagingGetPhysAddr(pageDirectory->tablesAddr);
    return pageDirectory;
}

static int checkPageAllowed(struct PageDirectory *pageDirectory, u32 index) {
    return kernelPageDirectory == pageDirectory ||
           (pageDirectory->tablesAddr[index] != kernelPageDirectory->tablesAddr[index]);
}

void pagingDestroyPageDirectory(struct PageDirectory *pageDirectory) {
    if (pageDirectory == kernelPageDirectory) {
        kSerialPrintf("Can not free kernel page directory\n");
        return;
    }

    LOG("Destroy page directory\n");

    if (pageDirectory == currentPageDirectory)
        pagingSwitchPageDirectory(kernelPageDirectory);

    for (u32 i = 0; i < NB_TABLE; i++) {
        if (pageDirectory->tablesInfo[i] && checkPageAllowed(pageDirectory, i)) {
            for (u32 j = 0; j < NB_PAGE; j++) {
                u32 physAddress = pageDirectory->tablesInfo[i]->pages[j] & 0xFFFFF000;

                if (physAddress)
                    freePhysicalMemory(physAddress);
            }
            kfree(pageDirectory->tablesInfo[i]);
        }
    }
    kfree(pageDirectory);
}

void pagingSwitchPageDirectory(struct PageDirectory *pageDirectory) {
    if (pageDirectory == currentPageDirectory)
        return;

    LOG("Switching page directory\n");
    currentPageDirectory = pageDirectory;
    asm volatile("mov %0, %%cr3" : : "r" (pageDirectory->physAddr));
}

static int allocPages(struct PageDirectory *pd, void *addr, u32 pages) {
    for (u32 i = 0; i < pages; i++) {
        u32 pageNumber = (u32) addr / PAGESIZE + i;

        struct TableDirectory *pt = pd->tablesInfo[pageNumber / NB_TABLE];
        if (!pt) {
            pt = kmalloc(sizeof(struct TableDirectory), PAGESIZE, "newTableDirectory");
            if (!pt) {
                freePages(pd, addr, i * PAGESIZE);
                return 0;
            }

            memset(pt, 0, sizeof(struct TableDirectory));
            pd->tablesInfo[pageNumber / NB_TABLE] = pt;
            pd->tablesAddr[pageNumber / NB_TABLE] = pagingGetPhysAddr(pt) | MEM_PRESENT | MEM_WRITE;
        } else {
            if (pt->pages[pageNumber % NB_PAGE] & MEM_ALLOCATED) {
                kSerialPrintf("Page already allocated: %u\n", pageNumber);
                freePages(pd, addr, i * PAGESIZE);
                return 0;
            }

            if (checkPageAllowed(pd, pageNumber / NB_TABLE) == 0) {
                kSerialPrintf("Alloc page not allowed: %u\n", pageNumber);
                freePages(pd, addr, i * PAGESIZE);
                return 0;
            }
        }

        pt->pages[pageNumber % NB_PAGE] = MEM_ALLOCATED;
    }
    return 1;
}

static void freePages(struct PageDirectory *pd, void *addr, u32 pages) {
    for (u32 i = 0; i < pages; i++) {
        u32 pageNumber = (u32) addr / PAGESIZE + i;

        if (!pd->tablesInfo[pageNumber / NB_TABLE])
            return;

        if (checkPageAllowed(pd, pageNumber / NB_TABLE) == 0) {
            kSerialPrintf("Alloc page not allowed: %u\n", pageNumber);
            return;
        }

        pd->tablesInfo[pageNumber / NB_TABLE]->pages[pageNumber % NB_PAGE] = 0;

        if (pd->tablesInfo[pageNumber / NB_TABLE] == currentPageDirectory->tablesInfo[pageNumber / NB_TABLE])
            invlpg((void *) (pageNumber * PAGESIZE));
    }
}

static int mapPagesToFrames(struct PageDirectory *pd, void *vaddr, u32 paddr, enum MEMFLAGS flags) {
    u32 pageNumber = (u32) vaddr / PAGESIZE;
    struct TableDirectory *pt = pd->tablesInfo[pageNumber / NB_TABLE];

    if (!pt || !(pt->pages[pageNumber % NB_PAGE] & MEM_ALLOCATED)) {
        kSerialPrintf("Page not allocated: %u\n", pageNumber);
        return 0;
    }

    if (checkPageAllowed(pd, pageNumber / NB_TABLE) == 0) {
        kSerialPrintf("Alloc page not allowed: %u\n", pageNumber);
        return 0;
    }

    pd->tablesAddr[pageNumber / NB_TABLE] = pagingGetPhysAddr(pt) | MEM_PRESENT | MEM_WRITE;
    pd->tablesAddr[pageNumber / NB_TABLE] |= (flags & (~MEM_NOTLBUPDATE));

    pt->pages[pageNumber % NB_PAGE] = paddr | flags | MEM_PRESENT | MEM_ALLOCATED;

    if (pt == currentPageDirectory->tablesInfo[pageNumber / NB_TABLE])
        invlpg(vaddr);

    return 1;
}

static int pagingAllocRec(struct PageDirectory *pd, u32 index, u32 pages, void *addr, enum MEMFLAGS flags) {
    u32 frame = allocPhysicalMemory();

    if (!mapPagesToFrames(pd, addr + index * PAGESIZE, frame, flags)) {
        kSerialPrintf("mapPagesToFrames(%X, %X, %X, %X) failed.\n", (u32) pd, (u32) (addr + index * PAGESIZE),
                      frame, flags);
        kSerialPrintf("info: %d - %d\n", index, pages);
        freePhysicalMemory(frame);
        return -1;
    }

    if (index >= pages - 1)
        return 0;

    int tmp = pagingAllocRec(pd, index + 1, pages, addr, flags);
    if (tmp != 0)
        freePhysicalMemory(frame);
    return tmp;
}

int pagingAlloc(struct PageDirectory *pd, void *addr, u32 size, enum MEMFLAGS flags) {
    LOG("pagingAlloc: %p - %u\n", addr, size);

    if (((u32) addr) % PAGESIZE != 0) {
        kSerialPrintf("addr not page aligned\n");
        return -1;
    }

    if (size % PAGESIZE != 0) {
        kSerialPrintf("size not page aligned\n");
        return -1;
    }

    u32 pages = size / PAGESIZE;

    if (!allocPages(pd, addr, pages)) {
        kSerialPrintf("allocPages(%X, %X, %u) failed.\n", (u32) pd, (u32) addr, (u32) pages);
        return -2;
    }

    if (pagingAllocRec(pd, 0, pages, addr, flags) != 0) {
        freePages(pd, addr, pages);
        return -3;
    }

    return 0;
}

void pagingFree(struct PageDirectory *pd, void *virtAddress, u32 size) {
    LOG("pagingFree: %p - %u\n", addr, size);

    if (((u32) virtAddress) % PAGESIZE != 0) {
        kSerialPrintf("addr not page aligned\n");
        return;
    }

    if (size % PAGESIZE != 0) {
        kSerialPrintf("size must be page aligned\n");
        return;
    }

    u32 pageNumber = (u32) virtAddress / PAGESIZE;
    while (size) {
        if (!pd->tablesInfo[pageNumber / NB_PAGE] || checkPageAllowed(pd, pageNumber / NB_TABLE) == 0)
            return;

        u32 physAddress = pagingGetPhysAddr(virtAddress);

        freePages(pd, virtAddress, 1);
        freePhysicalMemory(physAddress);

        pageNumber++;
        size -= PAGESIZE;
    }
}

int pagingSetFlags(struct PageDirectory *pd, void *addr, u32 size, enum MEMFLAGS flags) {
    if (((u32) addr) % PAGESIZE != 0) {
        kSerialPrintf("paging_setFlags: addr not page aligned\n");
        return -1;
    }

    if (size % PAGESIZE != 0) {
        kSerialPrintf("size not page aligned\n");
        return -1;
    }

    for (u32 done = 0; done < size / PAGESIZE; done++) {
        u32 pageNumber = (u32) addr / PAGESIZE + done;
        if (!pd->tablesInfo[pageNumber / NB_TABLE] ||
            !(pd->tablesInfo[pageNumber / NB_TABLE]->pages[pageNumber % NB_PAGE] & MEM_ALLOCATED)) {
            kSerialPrintf("page not init\n");
            return -2;
        }

        if (checkPageAllowed(pd, pageNumber / NB_PAGE) == 0) {
            kSerialPrintf("Alloc page not allowed: %u\n", pageNumber);
            return 0;
        }

        u32 *page = &pd->tablesInfo[pageNumber / NB_TABLE]->pages[pageNumber % NB_PAGE];
        *page = (*page & 0xFFFFF000) | flags | MEM_PRESENT | MEM_ALLOCATED;

        if (pd->tablesInfo[pageNumber / NB_TABLE] == currentPageDirectory->tablesInfo[pageNumber / NB_PAGE])
            invlpg((void *) (pageNumber * PAGESIZE));
    }

    return 0;
}
