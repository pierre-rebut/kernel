//
// Created by rebut_p on 16/11/18.
//

#include <string.h>
#include <stdio.h>
#include "paging.h"
#include "allocator.h"
#include "physical-memory.h"

struct PageDirectory *kernelPageDirectory = NULL;
struct PageDirectory *currentPageDirectory = NULL;

static int allocPages(struct PageDirectory *pd, void *addr, u32 pages);
static void freePages(struct PageDirectory *pd, void *addr, u32 pages);

void initPaging(u32 memSize) {
    kernelPageDirectory = kmalloc(sizeof(struct PageDirectory), PAGESIZE);
    memset(kernelPageDirectory, 0, sizeof(struct PageDirectory) - 4);
    kernelPageDirectory->physAddr = (u32) kernelPageDirectory;

    u32 addr = 0;
    for (u8 i = 0; i < 3; i++) {

        kernelPageDirectory->tablesInfo[i] = kmalloc(sizeof(struct TableDirectory), PAGESIZE);
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

    u32 kernelpts = memSize / PAGESIZE / NB_PAGE;
    struct TableDirectory *heap_pts = kmalloc(kernelpts * sizeof(struct TableDirectory), PAGESIZE);
    memset(heap_pts, 0, kernelpts * sizeof(struct TableDirectory));

    for (u32 i = 0; i < kernelpts; i++) {
        kernelPageDirectory->tablesInfo[KERNEL_HEAP_START / PAGESIZE / NB_TABLE + i] = heap_pts + i;
        kernelPageDirectory->tablesAddr[KERNEL_HEAP_START / PAGESIZE / NB_TABLE + i] =
                (u32) (heap_pts + i) | MEM_PRESENT;
    }

    switchPageDirectory(kernelPageDirectory);

    u32 cr0;
    asm volatile ("movl %%cr0, %0": "=r"(cr0));
    cr0 |= (1 << 31) | (1 << 16);
    asm volatile ("movl %0, %%cr0": :"a"(cr0));
}

static inline void invlpg(void *p) {
    asm volatile("invlpg (%0)"::"r" (p) : "memory");
}

static u32 getPhysAddr(const void *vaddr) {
    u32 pageNumber = (u32) vaddr / PAGESIZE;
    struct TableDirectory *pt = currentPageDirectory->tablesInfo[pageNumber / NB_TABLE];
    if (!pt)
        return 0;

    printf("\nvirt-->phys: pagenr: %u, pt: %Xh\n", pageNumber, (u32) pt);
    return ((pt->pages[pageNumber % NB_PAGE] & 0xFFFFF000) + ((u32) vaddr & 0x00000FFF));
}

struct PageDirectory *createPageDirectory() {
    struct PageDirectory *pageDirectory = kmalloc(sizeof(struct PageDirectory), PAGESIZE);
    if (!pageDirectory)
        return NULL;

    memcpy(pageDirectory, kernelPageDirectory, sizeof(struct PageDirectory) - 4);
    pageDirectory->physAddr = getPhysAddr(pageDirectory->tablesAddr);
    return pageDirectory;
}

static int checkPageAllowed(struct PageDirectory *pageDirectory, u32 index) {
    return kernelPageDirectory == pageDirectory ||
           (pageDirectory->tablesAddr[index] != kernelPageDirectory->tablesAddr[index]);
}

void destroyPageDirectory(struct PageDirectory *pageDirectory) {
    if (pageDirectory == kernelPageDirectory) {
        printf("Can not free kernel page directory\n");
        return;
    }

    if (pageDirectory == currentPageDirectory)
        switchPageDirectory(kernelPageDirectory);

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

void switchPageDirectory(struct PageDirectory *pageDirectory) {
    if (pageDirectory == currentPageDirectory)
        return;

    printf("Switching page directory\n");
    currentPageDirectory = pageDirectory;
    asm volatile("mov %0, %%cr3" : : "r"(pageDirectory->physAddr));
}

static int allocPages(struct PageDirectory *pd, void *addr, u32 pages) {
    for (u32 i = 0; i < pages; i++) {
        u32 pageNumber = (u32) addr / PAGESIZE + i;

        struct TableDirectory *pt = pd->tablesInfo[pageNumber / NB_TABLE];
        if (!pt) {
            pt = kmalloc(sizeof(struct TableDirectory), PAGESIZE);
            if (!pt) {
                freePages(pd, addr, i * PAGESIZE);
                return 0;
            }

            memset(pt, 0, sizeof(struct TableDirectory));
            pd->tablesInfo[pageNumber / NB_TABLE] = pt;
            pd->tablesAddr[pageNumber / NB_TABLE] = getPhysAddr(pt) | MEM_PRESENT | MEM_WRITE;
        } else {
            if (pt->pages[pageNumber % NB_PAGE] & MEM_ALLOCATED) {
                printf("Page already allocated: %u\n", pageNumber);
                freePages(pd, addr, i * PAGESIZE);
                return 0;
            }

            if (checkPageAllowed(pd, pageNumber / NB_TABLE) == 0) {
                printf("Alloc page not allowed: %u\n", pageNumber);
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
            printf("Alloc page not allowed: %u\n", pageNumber);
            return;
        }

        pd->tablesInfo[pageNumber / NB_TABLE]->pages[pageNumber % NB_PAGE] = 0;

        if (pd->tablesInfo[pageNumber / NB_TABLE] == currentPageDirectory->tablesInfo[pageNumber / NB_TABLE])
            invlpg((void *) (pageNumber * PAGESIZE));
    }
}

static int mapPagesToFrames(struct PageDirectory *pd, void *vaddr, u32 paddr, u32 pages, enum MEMFLAGS flags) {
    for (u32 i = 0; i < pages; i++) {
        u32 pageNumber = (u32) vaddr / PAGESIZE + i;
        struct TableDirectory *pt = pd->tablesInfo[pageNumber / NB_TABLE];

        if (!pt || !(pt->pages[pageNumber % NB_PAGE] & MEM_ALLOCATED)) {
            printf("Page not allocated: %u\n", pageNumber);
            return 0;
        }

        if (checkPageAllowed(pd, pageNumber / NB_TABLE) == 0) {
            printf("Alloc page not allowed: %u\n", pageNumber);
            return 0;
        }

        pd->tablesAddr[pageNumber / NB_TABLE] = getPhysAddr(pt) | MEM_PRESENT | MEM_WRITE;
        pd->tablesAddr[pageNumber / NB_TABLE] |= (flags & (~MEM_NOTLBUPDATE)); // Update codes

        pt->pages[pageNumber % NB_PAGE] = (paddr + i * PAGESIZE) | flags | MEM_PRESENT | MEM_ALLOCATED;

        if (pt == currentPageDirectory->tablesInfo[pageNumber / NB_TABLE])
            invlpg(vaddr + i * PAGESIZE);

        if (flags & MEM_USER)
            printf("page %u now associated to physAddress %Xh\n", pageNumber, paddr + i * PAGESIZE);
    }
    return 1;
}

static int pagingAllocRec(struct PageDirectory *pd, u32 index, u32 pages, void *addr, enum MEMFLAGS flags) {
    u32 frame = allocPhysicalMemory();

    if (!mapPagesToFrames(pd, addr + index * PAGESIZE, frame, 1, flags)) {
        printf("mapPagesToFrames(%X, %X, %X, %u, %X) failed.\n", (u32) pd, (u32) (addr + index * PAGESIZE),
               frame, 1, flags);
        printf("info: %d - %d\n", index, pages);
        freePhysicalMemory(frame);
        return -1;
    }

    if (flags & MEM_USER)
        printf("page now allocated: %u physAddress: %Xh\n", index, frame);

    if (index >= pages - 1)
        return 0;

    int tmp = pagingAllocRec(pd, index + 1, pages, addr, flags);
    if (tmp != 0)
        freePhysicalMemory(frame);
    return tmp;
}

int pagingAlloc(struct PageDirectory *pd, void *addr, u32 size, enum MEMFLAGS flags) {
    if (((u32) addr) % PAGESIZE != 0) {
        printf("addr not page aligned\n");
        return -1;
    }

    if (size % PAGESIZE != 0) {
        printf("size not page aligned\n");
        return -1;
    }

    u32 pages = size / PAGESIZE;

    if (!allocPages(pd, addr, pages)) {
        printf("allocPages(%X, %X, %u) failed.\n", (u32) pd, (u32) addr, (u32) pages);
        return -2;
    }

    if (pagingAllocRec(pd, 0, pages, addr, flags) != 0)
        return -3;

    return 0;
}

void pagingFree(struct PageDirectory *pd, void *virtAddress, u32 size) {
    if (((u32) virtAddress) % PAGESIZE != 0) {
        printf("addr not page aligned\n");
        return;
    }

    if (size % PAGESIZE != 0) {
        printf("size must be page aligned\n");
        return;
    }

    u32 pageNumber = (u32) virtAddress / PAGESIZE;
    while (size) {
        if (!pd->tablesInfo[pageNumber / NB_PAGE] || checkPageAllowed(pd, pageNumber / NB_TABLE) == 0)
            return;

        u32 physAddress = getPhysAddr(virtAddress);

        freePages(pd, virtAddress, 1);
        freePhysicalMemory(physAddress);

        pageNumber++;
        size -= PAGESIZE;
    }
}

int pagingSetFlags(struct PageDirectory *pd, void *addr, u32 size, enum MEMFLAGS flags) {
    if (((u32) addr) % PAGESIZE != 0) {
        printf("paging_setFlags: addr not page aligned\n");
        return -1;
    }

    if (size % PAGESIZE != 0) {
        printf("size not page aligned\n");
        return -1;
    }

    for (u32 done = 0; done < size / PAGESIZE; done++) {
        u32 pageNumber = (u32) addr / PAGESIZE + done;
        if (!pd->tablesInfo[pageNumber / NB_TABLE] ||
            !(pd->tablesInfo[pageNumber / NB_TABLE]->pages[pageNumber % NB_PAGE] & MEM_ALLOCATED)) {
            printf("page not init\n");
            return -2;
        }

        if (checkPageAllowed(pd, pageNumber / NB_PAGE) == 0) {
            printf("Alloc page not allowed: %u\n", pageNumber);
            return 0;
        }

        u32 *page = &pd->tablesInfo[pageNumber / NB_TABLE]->pages[pageNumber % NB_PAGE];
        *page = (*page & 0xFFFFF000) | flags | MEM_PRESENT | MEM_ALLOCATED;

        if (pd->tablesInfo[pageNumber / NB_TABLE] == currentPageDirectory->tablesInfo[pageNumber / NB_PAGE])
            invlpg((void *) (pageNumber * PAGESIZE));
    }

    return 0;
}
