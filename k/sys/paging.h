#ifndef PAGING_H
#define PAGING_H

#include <k/types.h>

#define IDMAP 3
#define PLACEMENT_BEGIN   0x600000   // 6 MiB
#define PLACEMENT_END     0xC00000   // 12 MiB

// memory location for MMIO of devices (networking card, EHCI, grafics card, ...)
#define PCI_MEM_START     0xC0000000 // 3 GiB
#define PCI_MEM_END       0xE0000000 // 3,5 GiB

// Where the kernel's private data (heap) is stored (virtual addresses)
#define KERNEL_HEAP_START 0xE0000000 // 3,5 GiB
#define KERNEL_HEAP_END   0xFFFFFFFF // 4 GiB - 1

#define PAGESIZE   0x1000 // Size of one page in bytes
#define PAGE_COUNT 1024   // Number of pages per page table
#define PT_COUNT   1024   // Number of page tables per page directory

#define CR0_PAGINGENABLED    1 << 31
#define CR0_WRITEPROTECT     1 << 16

#define IA32_MTRRCAP 0xFE
#define IA32_MTRR_DEF_TYPE 0x2FF
#define IA32_MTRR_FIX64K_00000 0x250
#define IA32_MTRR_FIX16K_80000 0x258
#define IA32_MTRR_FIX4K_C0000 0x268
#define IA32_MTRR_PHYSBASE0 0x200
#define IA32_MTRR_PHYSMASK0 0x201

typedef enum {
    MEM_PRESENT = 1,
    MEM_WRITE = 1 << 1,
    MEM_USER = 1 << 2,
    MEM_WRITETHROUGH = 1 << 3,
    MEM_NOCACHE = 1 << 4,
    MEM_NOTLBUPDATE = 1 << 8,
    MEM_ALLOCATED = 1 << 9, // PrettyOS-specific. Indicates if a page is allocated or not
    MEM_CONTINUOUS = 1 << 10 // PrettyOS-specific. Used as function argument to request physically continuous memory
} MEMFLAGS_t;

typedef enum {
    MTRR_UNCACHABLE = 0x00,
    MTRR_WRITECOMBINING = 0x01,
    MTRR_WRITETHROUGH = 0x04,
    MTRR_WRITEPROTECTED = 0x05,
    MTRR_WRITEBACK = 0x06
} MTRR_CACHETYPE;

// Memory Map
typedef struct {
    u32 mysize; // Size of this entry
    u64 base;   // The region's address
    u64 size;   // The region's size
    u32 type;   // Is "1" for "free"
} __attribute__((packed)) memoryMapEntry_t;

// Paging
typedef struct {
    u32 pages[PAGE_COUNT];
} __attribute__((packed)) pageTable_t;

typedef struct {
    u32 codes[PT_COUNT];
    pageTable_t *tables[PT_COUNT];
    u32 physAddr;
} __attribute__((packed)) pageDirectory_t;


extern pageDirectory_t *kernelPageDirectory;
extern pageDirectory_t *currentPageDirectory;


u32 paging_install(memoryMapEntry_t *memoryMapBegin, size_t memoryMapLength);

// Management of physical memory
u32 paging_allocPhysMem(size_t pages, int allowMove);

int paging_allocPhysAddr(u32 addr, size_t pages);

void paging_freePhysMem(u32 addr, size_t pages);

void paging_setPhysMemCachingBehaviour(u32 start, size_t pages, MTRR_CACHETYPE);

// Management of virtual memory
void *paging_allocVirtMem(pageDirectory_t *pd, size_t pages);

int paging_allocVirtAddr(pageDirectory_t *pd, void *addr, size_t pages);

void paging_freeVirtMem(pageDirectory_t *pd, void *addr, size_t pages);

// Linkage betweeen physical and virtual memory
int paging_mapVirtToPhysAddr(pageDirectory_t *pd, void *vaddr, u32 paddr, size_t pages, MEMFLAGS_t flags);

void paging_setFlags(pageDirectory_t *pd, void *addr, u32 size, MEMFLAGS_t flags);

u32 getPhysAddr(const void *vaddr);

// High level functions
int paging_alloc(pageDirectory_t *pd, void *addr, u32 size, MEMFLAGS_t flags);

int paging_allocNew(pageDirectory_t *pd, void *addr, u32 size, MEMFLAGS_t flags);

void paging_free(pageDirectory_t *pd, void *addr, u32 size);

void *paging_allocMMIO(u32 paddr, size_t pages);

// Page directory management
pageDirectory_t *paging_createPageDirectory(void);

void paging_destroyPageDirectory(pageDirectory_t *pd);

void paging_switch(pageDirectory_t *pd);

#endif
