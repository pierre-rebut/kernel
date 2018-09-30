//
// Created by rebut_p on 30/09/18.
//

#include "paging.h"
#include "kalloc.h"

#include <error.h>
#include <utils.h>

#include <stdio.h>
#include <string.h>

#define FOUR_GB 0x100000000ull // Highest address + 1


pageDirectory_t *kernelPageDirectory;
pageDirectory_t *currentPageDirectory = 0;

extern char _kernel_beg, _kernel_end; // defined in linker script
extern char _ro_start, _ro_end;       // defined in linker script

static u32 MAX_DWORDS = 0;        // Can be FOUR_GB / PAGESIZE / 32 at maximum;
static u32 *phys_reservationTable; // 0: free;      1: used
static u32 *phys_blockingTable;    // 0: unblocked; 1: blocked (not movable)
static u32 firstFreeDWORD =
        PLACEMENT_END / PAGESIZE / 32; // Exclude the first 12 MiB from being allocated (needed for DMA later on)


static u32 physMemInit(memoryMapEntry_t *memoryMapBegin, size_t memoryMapLength);

#define _DIAGNOSIS_


u32 paging_install(memoryMapEntry_t *memoryMapBegin, size_t memoryMapLength) {
    u32 ram_available = physMemInit(memoryMapBegin, memoryMapLength);

    // Setup the kernel page directory
    kernelPageDirectory = malloc(sizeof(pageDirectory_t), PAGESIZE, "pag-kernelPD");
    memset(kernelPageDirectory, 0, sizeof(pageDirectory_t) - 4);
    kernelPageDirectory->physAddr = (u32) kernelPageDirectory;

#ifdef _DIAGNOSIS_
    printf("\nkernelPageDirectory (virt., phys.): %Xh, %Xh\n", kernelPageDirectory, kernelPageDirectory->physAddr);
#endif

    // Setup the page tables for 0 MiB - 12 MiB, identity mapping
    u32 addr = 0;
    for (u8 i = 0; i < IDMAP; i++) {
        // Page directory entry, virt=phys due to placement allocation in id-mapped area
        kernelPageDirectory->tables[i] = malloc(sizeof(pageTable_t), PAGESIZE, "pag-kernelPT");
        kernelPageDirectory->codes[i] = (u32) kernelPageDirectory->tables[i] | MEM_PRESENT | MEM_WRITE;

        // Page table entries, identity mapping
        for (u32 j = 0; j < PAGE_COUNT; ++j) {
            u32 flags;
            if (addr <
                0x100000) // The first MiB has to be flushed from TLB, since VM86 adds MEM_USER and breaks identity mapping
                flags = MEM_PRESENT | MEM_WRITE | MEM_ALLOCATED;
            else
                flags = MEM_PRESENT | MEM_WRITE | MEM_NOTLBUPDATE | MEM_ALLOCATED;
            kernelPageDirectory->tables[i]->pages[j] = addr | flags;
            addr += PAGESIZE;
        }
    }

    kernelPageDirectory->tables[0]->pages[0] = 0 | MEM_PRESENT | MEM_ALLOCATED; // Make first page read-only

    // Setup the page tables for kernel heap (3,5 - 4 GiB), unmapped
    size_t kernelpts = min(PT_COUNT / 8, ram_available / PAGESIZE /
                                         PAGE_COUNT); // Do not allocate more PTs than necessary (limited by available memory)
    pageTable_t *heap_pts = malloc(kernelpts * sizeof(pageTable_t), PAGESIZE, "kheap_pts");
    memset(heap_pts, 0, kernelpts * sizeof(pageTable_t));
    for (u32 i = 0; i < kernelpts; i++) {
        kernelPageDirectory->tables[KERNEL_HEAP_START / PAGESIZE / PAGE_COUNT + i] = heap_pts + i;
        kernelPageDirectory->codes[KERNEL_HEAP_START / PAGESIZE / PAGE_COUNT + i] = (u32) (heap_pts + i) | MEM_PRESENT;
    }

    // Make some parts of the kernel (Sections text and rodata) read-only
    u32 startpt = ((u32) &_ro_start) / PAGESIZE / PAGE_COUNT; // Page table, where read-only section starts
    u32 startp = ((u32) &_ro_start) / PAGESIZE % PAGE_COUNT; // Page, where read-only section starts
    if ((u32) &_ro_start % PAGESIZE != 0) {
        startp++;
        if (startp > PAGE_COUNT) {
            startpt++;
            startp = 0;
        }
    }
    u32 endpt = ((u32) &_ro_end) / PAGESIZE / PAGE_COUNT; // Page table, where read-only section ends
    u32 endp = ((u32) &_ro_end) / PAGESIZE % PAGE_COUNT; // Page, where read-only section ends
    if (endp > 0)
        endp--;
    else {
        endp = PAGE_COUNT - 1;
        endpt--;
    }
    for (u32 i = startpt; i <= endpt; i++) {
        for (u32 j = startp; j <= endp; j++) {
            kernelPageDirectory->tables[i]->pages[j] &= (~MEM_WRITE); // Forbid writing
        }
    }

    // Tell CPU to enable paging
    paging_switch(kernelPageDirectory);

    u32 cr0;
    asm volatile ("movl %%cr0, %0": "=r"(cr0)); // read CR0

    printf("cr0 val: %X\n", cr0);
    cr0 |= CR0_PAGINGENABLED | CR0_WRITEPROTECT;
    printf("cr0 val: %X\n", cr0);
    asm volatile("movl %0, %%cr0"::"a"(cr0)); // write CR0

    return (ram_available);
}


static int
isMemoryMapAvailable(const memoryMapEntry_t *memoryMapBegin, const memoryMapEntry_t *memoryMapEnd, u64 beg, u64 end) {
    u64 covered = beg;
    for (const memoryMapEntry_t *outerLoop = memoryMapBegin;
         outerLoop < memoryMapEnd; outerLoop = (memoryMapEntry_t *) ((char *) outerLoop + outerLoop->mysize + 4)) {
        // There must not be an "reserved" entry which reaches into the specified area
        if ((outerLoop->type != 1) && (outerLoop->base < end) && ((outerLoop->base + outerLoop->size) > beg)) {
            return (0);
        }
        // Check whether the "free" entries cover the whole specified area.
        for (const memoryMapEntry_t *entry = memoryMapBegin;
             entry < memoryMapEnd; entry = (memoryMapEntry_t *) ((char *) entry + entry->mysize + 4)) {
            if (entry->base <= covered && (entry->base + entry->size) > covered) {
                covered = entry->base + entry->size;
            }
        }
    }

    // Return whether the whole area is covered by "free" entries
    return (covered >= end);
}

static void physSetBits(u32 addrBegin, u32 addrEnd, int reserved) {
    // Calculate the bit-numbers
    u32 start = alignUp(addrBegin, PAGESIZE) / PAGESIZE;
    u32 end = alignDown(addrEnd, PAGESIZE) / PAGESIZE;

    // Set all these bits
    for (u32 j = start; j < end; ++j) {
        if (reserved)
                SET_BIT(phys_reservationTable[j / 32], j % 32);
        else
                CLEAR_BIT(phys_reservationTable[j / 32], j % 32);
    }
}

static u32 physMemInit(memoryMapEntry_t *memoryMapBegin, size_t memoryMapLength) {
    memoryMapEntry_t *memoryMapEnd = (memoryMapEntry_t *) ((char *) memoryMapBegin + memoryMapLength);
    printf("Memory map (%X -> %X):\n", memoryMapBegin, memoryMapEnd);

    // Prepare the memory map entries, since we work with max 4 GB only. The last entry in the entry-array has size 0.
    for (memoryMapEntry_t *entry = memoryMapBegin;
         entry < memoryMapEnd; entry = (memoryMapEntry_t *) ((char *) entry + entry->mysize + 4)) {
        printf("%Xh -> %Xh %u (%u Bytes)\n", (u32) entry->base, (u32)(entry->base + entry->size),
               entry->type, (u32) entry->size); // Print the memory map

        // We will completely ignore memory above 4 GB or with size of 0, move following entries backward by one then
        if (entry->base < FOUR_GB && entry->size != 0) {
            // Eventually reduce the size so the the block doesn't exceed 4 GB
            if (entry->base + entry->size >= FOUR_GB) {
                entry->size = FOUR_GB - entry->base;
            }

            if (entry->type == 1)
                MAX_DWORDS = max(MAX_DWORDS, (u32)(entry->base + entry->size) / PAGESIZE / 32); // Calculate required size of bittables
        }
    }

    // Check that 6 MiB-12 MiB is free for use
    if (!isMemoryMapAvailable(memoryMapBegin, memoryMapEnd, PLACEMENT_BEGIN, IDMAP * PAGE_COUNT * PAGESIZE))
        kpanic("The memory between 6 MiB and 12 MiB is not free for use. OS halted!");

    // We store our data here, initialize all bits to "reserved"
    phys_reservationTable = malloc(MAX_DWORDS * 4, 0, "phys_reservationTable");
    memset(phys_reservationTable, 0xFF, MAX_DWORDS * 4);
    phys_blockingTable = malloc(MAX_DWORDS * 4, 0, "phys_blockingTable");

    // Set the bitmap bits according to the memory map now. "type==1" means "free".
    for (const memoryMapEntry_t *entry = memoryMapBegin;
         entry < memoryMapEnd; entry = (memoryMapEntry_t *) ((char *) entry + entry->mysize + 4)) {
        if (entry->type == 1 && entry->base < FOUR_GB) // Set bits to "free"
        {
            physSetBits(entry->base, entry->base + entry->size, 0);
        }
    }

    // Find the number of dwords we can use, skipping the last, "reserved"-only ones
    u32 dwordCount = 0;

    for (u32 i = 0; i < MAX_DWORDS; i++) {
        if (phys_reservationTable[i] != 0xFFFFFFFF) {
            dwordCount = i + 1;
        }
    }

    // Reserve first 12 MiB
    physSetBits(0x00000000, PLACEMENT_END - 1, 1);

    // Reserve the region of the kernel code
    if ((u32) &_kernel_end >= PLACEMENT_END)
        physSetBits((u32) &_kernel_beg, (u32) &_kernel_end, 1);

    printf("Highest available RAM: %Xh\n", dwordCount * 32 * PAGESIZE);

    // Return the amount of memory available (or rather the highest address)
    return (dwordCount * 32 * PAGESIZE);
}

// ------------------------------------------------------------------------------
// Helper functions and inline assembly
// ------------------------------------------------------------------------------

void paging_switch(pageDirectory_t *pd) {
    if (pd != currentPageDirectory) // Switch page directory only if the new one is different from the old one
    {

        printf("\nDEBUG: paging_switch: pd=%X, pd->physAddr=%X\n", pd, pd->physAddr);

        currentPageDirectory = pd;
        asm volatile("mov %0, %%cr3" : : "r"(pd->physAddr));
    }
}

static inline void invalidateTLBEntry(void *p) {
    __asm__ volatile("invlpg (%0)"::"r" (p) : "memory");
}

static inline u32 getFirstFreeBit(u32 value) {
    // Find the number of first free bit.
    // This inline assembler instruction is smaller and faster than a C loop to identify this bit
    u32 bitnr;
    __asm__("bsfl %1, %0" : "=r"(bitnr) : "r"(value));
    return bitnr;
}

static int changeAllowed(pageDirectory_t *pd, size_t i) {
    return kernelPageDirectory == pd || (pd->tables[i] != kernelPageDirectory->tables[i]);
}

// ------------------------------------------------------------------------------
// Allocates a continuous area of pyhsical memory of a given number of pages. The
// function decides about the physical address. The physical address might change
// internally if the same memory is requested by paging_allocPhysAddr, as long as
// allowMove is true.
// ------------------------------------------------------------------------------
u32 paging_allocPhysMem(size_t pages, int allowMove) {
    // Search for a continuous block of free pages
    u32 dword = firstFreeDWORD;
    for (; dword < MAX_DWORDS; dword++) {
        if (phys_reservationTable[dword] != 0xFFFFFFFF) {
            int aquirable = 1;
            u32 bitnr = getFirstFreeBit(~phys_reservationTable[dword]) + dword * 32;
            u32 bitnr_old = bitnr;
            // Check if here is enough space
            for (size_t i = 0; i < pages && aquirable; i++) {
                aquirable = !(phys_reservationTable[bitnr / 32] & (1 << (bitnr % 32)));
                bitnr++;
            }
            if (!aquirable)
                continue; // Not enough space - continue searching

            bitnr = bitnr_old;
            for (size_t i = 0; i < pages; i++) {
                SET_BIT(phys_reservationTable[bitnr / 32], bitnr % 32); // Reserve memory
                if (allowMove)
                        CLEAR_BIT(phys_blockingTable[bitnr / 32], bitnr % 32); // Don't protect against moving
                else
                        SET_BIT(phys_blockingTable[bitnr / 32], bitnr % 32); // Protect against moving
                bitnr++;
            }

            return (bitnr_old * PAGESIZE);
        }
    }

    // No free page found
    return (0);
}

// ------------------------------------------------------------------------------
// Allocates an area of pyhsical memory of a given number of pages. The address
// is given. If the requested memory is in use, it will be moved away if possible.
// ------------------------------------------------------------------------------
int paging_allocPhysAddr(u32 addr, size_t pages) {
    // look for addr
    u32 bitnr = addr / PAGESIZE;
    int aquirable = 1;

    for (size_t i = 0; i < pages && aquirable; i++) {
        aquirable = !(phys_reservationTable[bitnr / 32] & (1 << (bitnr % 32)));
        // Later: aquirable = !(phys_reservationTable[bitnr / 32] & BIT(bitnr % 32)) || !(phys_blockingTable[bitnr / 32] & BIT(bitnr % 32));
        // The memory in question could be moved to a different page (if allowed)
        bitnr++;
    }

    if (!aquirable) {
        return 0;
    }

    // allocate
    bitnr = addr / PAGESIZE;
    for (size_t i = 0; i < pages; i++) {
        // TODO: Move old content before writing, if (phys_reservationTable[bitnr / 32] & BIT(bitnr % 32)).
        SET_BIT(phys_reservationTable[bitnr / 32], bitnr % 32); // Reserve memory
        SET_BIT(phys_blockingTable[firstFreeDWORD], bitnr % 32); // Protect against moving
        bitnr++;
    }

    return 1;
}

// ------------------------------------------------------------------------------
// Frees physical memory allocated by paging_allocPhysAddr or paging_allocPhysMem.
// ------------------------------------------------------------------------------
void paging_freePhysMem(u32 addr, size_t pages) {
    // Calculate the number of the bit
    u32 bitnr = addr / PAGESIZE;

    // Maybe the affected dword (which has a free bit now) is ahead of firstFreeDWORD?
    if (bitnr / 32 < firstFreeDWORD) {
        firstFreeDWORD = bitnr / 32;
    }

    // Set the page to "free"
    CLEAR_BIT(phys_reservationTable[bitnr / 32], bitnr % 32);
    if (pages > 1)
        paging_freePhysMem(addr + PAGESIZE, pages - 1);
}

/*

// ------------------------------------------------------------------------------
// Access MTRRs (if available) to optimize caching behaviour
// ------------------------------------------------------------------------------
static void setFixedMTRR(u32 firstMSR, u16 counter, size_t pages, MTRR_CACHETYPE behaviour) {
    u64 msrValue = cpu_MSRread(firstMSR + counter / 8);
#ifdef _PAGING_DIAGNOSIS_
    printf("\nReplace MSR %x: %X%X ", firstMSR + counter / 8, (uint32_t)(msrValue >> 32), (uint32_t)msrValue);
#endif
    for (size_t i = 0; i < pages; i++) {
        msrValue &= ~((u64) 0xFF << (counter % 8) * 8); // clear existing behaviour
        msrValue |= ((u64) behaviour) << ((counter % 8) * 8);// and replace it by desired behaviour
        if ((counter + 1) % 8 == 0 || i == pages - 1) {
#ifdef _PAGING_DIAGNOSIS_
            printf("by %X%X", (uint32_t)(msrValue >> 32), (uint32_t)msrValue);
#endif
            cpu_MSRwrite(firstMSR + counter / 8, msrValue);
            if (i != pages - 1) {
                msrValue = cpu_MSRread(firstMSR + (counter + 1) / 8);
#ifdef _PAGING_DIAGNOSIS_
                printf("\nReplace MSR %x: %X%X ", firstMSR + (counter+1) / 8, (uint32_t)(msrValue >> 32), (uint32_t)msrValue);
#endif
            }
        }
        counter++;
    }
}

void paging_setPhysMemCachingBehaviour(u32 start, size_t pages, MTRR_CACHETYPE behaviour) {
    if (!cpu_supports(CF_MTRR))
        return;

    static u64 mtrr_feature = 0;
    if (mtrr_feature == 0)
        mtrr_feature = cpu_MSRread(IA32_MTRRCAP);
    u64 mtrr_defType = cpu_MSRread(IA32_MTRR_DEF_TYPE);

#ifdef _PAGING_DIAGNOSIS_
    printf("\nMTRR feature: %X, MTRR defType: %X", (uint32_t)mtrr_feature, (uint32_t)mtrr_defType);
#endif

    if (!(mtrr_defType & (1 << 11))) {
#ifdef _PAGING_DIAGNOSIS_
        printf("\nMTRRs disabled");
        return;
#endif
    }

    if (behaviour == MTRR_WRITECOMBINING && !(mtrr_feature & (1 << (10))))
        behaviour = MTRR_WRITETHROUGH; // Fall-back to Write-Through caching

    u32 cr0, cr4;
    // Disable caching before touching MTRRs
    __asm__ volatile(
    "cli;"
            "mov %%cr0, %%eax;"
            "mov %%eax, %0;"          // Store CR0
            "or  $0x40000000, %%eax;" // disable cache
            "and $0xDFFFFFFF, %%eax;" // disable enforced Write-Through cache
            "mov %%eax, %%cr0;"
            "mov %%cr4, %%eax;"
            "mov %%eax, %1;"          // Store CR4
            "and $0xFFFFFF7F, %%eax;" // deactivate PGE
            "mov %%eax, %%cr4;"
            "mov %%cr3, %%eax;"       // Flush TLB
            "mov %%eax, %%cr3;" : "=r"(cr0), "=r"(cr4) : : "eax");

    // Use fixed MTRRs for low adresses
    // TODO: Make it more flexible to support memory that spans over several (fixed) MTRR length types
    if (mtrr_feature & (1 << 8) && mtrr_defType & (1 << 10) && start <= 0xFFFFF) {
#ifdef _PAGING_DIAGNOSIS_
        printf("\nUsing fixed MTRR");
#endif
        if (start < 0x7FFFF && pages % 128 == 0)
            setFixedMTRR(IA32_MTRR_FIX64K_00000, start / 0x10000, pages / 16, behaviour);
        else if (start > 0x80000 && start < 0xBFFFF && pages % 4 == 0)
            setFixedMTRR(IA32_MTRR_FIX16K_80000, (start - 0x80000) / 0x4000, pages / 4, behaviour);
        else if (start > 0xC0000 && start < 0xFFFFF)
            setFixedMTRR(IA32_MTRR_FIX4K_C0000, (start - 0xC0000) / 0x1000, pages, behaviour);

        goto end;
    }

#ifdef _PAGING_DIAGNOSIS_
    printf("\nUsing variable MTRR");
    for (int i = 0; i < BYTE1(mtrr_feature); i++)
    {
        uint64_t physbase = cpu_MSRread(IA32_MTRR_PHYSBASE0 + 2 * i);
        uint64_t physmask = cpu_MSRread(IA32_MTRR_PHYSMASK0 + 2 * i);
        printf("\nMTRR %u: %X %X, %X %X", i, (uint32_t)(physbase >> 32), (uint32_t)physbase, (uint32_t)(physmask >> 32), (uint32_t)physmask);
    }
#endif

    pages = 0x1 << bsr(pages); // align down page count to power of 2
    // Use variable MTRRs for high adresses or in case fixed MTRRs are unsupported
    for (int i = BYTE1(mtrr_feature) - 1; i > 0; i--) {
        u64 physmask = cpu_MSRread(IA32_MTRR_PHYSMASK0 + 2 * i);
        if (!(physmask & (1 << 11))) // Free MTRR
        {
            u64 physbase = (start & 0xFFFFF000) | behaviour;
            physmask = (1 << 11) | (~(pages * PAGESIZE - 1) & 0xFFFFF000);
            cpu_MSRwrite(IA32_MTRR_PHYSBASE0 + 2 * i, physbase);
            cpu_MSRwrite(IA32_MTRR_PHYSMASK0 + 2 * i, physmask);
#ifdef _PAGING_DIAGNOSIS_
            printf("\nWriting:");
            printf("\nMTRR %u: %X %X, %X %X", i, (uint32_t)(physbase >> 32), (uint32_t)physbase, (uint32_t)(physmask >> 32), (uint32_t)physmask);
#endif
            break;
        }
    }

    end:
    // Enable caching again
    __asm__ volatile(
    "mov %%eax, %%cr0;"
            "mov %%ebx, %%cr4;"
            "sti": : "a"(cr0), "b"(cr4) : "ecx");
}

 */

// ------------------------------------------------------------------------------
// Allocates an area of virtual memory of a given number of pages in the given
// page directory. The function decides about the address, which is returned.
// ------------------------------------------------------------------------------
void *paging_allocVirtMem(pageDirectory_t *pd, size_t pages) {
    (void) pd;
    (void) pages;
    return 0; // TODO
}

// ------------------------------------------------------------------------------
// Allocates an area of virtual memory of a given number of pages in the given
// page directory. The address is given.
// ------------------------------------------------------------------------------
int paging_allocVirtAddr(pageDirectory_t *pd, void *addr, size_t pages) {
    // "addr" must be page-aligned
    ASSERT(((u32) addr) % PAGESIZE == 0);

    // We repeat allocating one page at once
    for (u32 done = 0; done < pages; done++) {
        u32 pagenr = (u32) addr / PAGESIZE + done;

        // Get the page table
        pageTable_t *pt = pd->tables[pagenr / PAGE_COUNT];
        if (!pt) {
            // Allocate the page table
            pt = malloc(sizeof(pageTable_t), PAGESIZE, "pageTable");
            if (!pt) {
                // Undo all allocations and return an error
                paging_freeVirtMem(pd, addr, done * PAGESIZE);
                return 0;
            }
            memset(pt, 0, sizeof(pageTable_t));
            pd->tables[pagenr / PAGE_COUNT] = pt;

            // Set physical address and flags
            pd->codes[pagenr / PAGE_COUNT] = getPhysAddr(pt) | MEM_PRESENT | MEM_WRITE;
        } else {
            // Maybe there is already memory allocated?
            if (pt->pages[pagenr % PAGE_COUNT] & MEM_ALLOCATED) {
                printf("Page already allocated: %u\n", pagenr);
                paging_freeVirtMem(pd, addr, done * PAGESIZE);
                return 0;
            }
            ASSERT(changeAllowed(pd, pagenr / PAGE_COUNT));
        }

        // Setup the page
        pt->pages[pagenr % PAGE_COUNT] = MEM_ALLOCATED;
    }
    return 1;
}

// ------------------------------------------------------------------------------
// Frees virtual memory allocated by paging_allocVirtMem or paging_allocVirtAddr.
// ------------------------------------------------------------------------------
void paging_freeVirtMem(pageDirectory_t *pd, void *addr, size_t pages) {
    // "addr" must be page-aligned
    ASSERT(((u32) addr) % PAGESIZE == 0);

    for (u32 pg = 0; pg < pages; ++pg) {
        u32 pagenr = (u32) addr / PAGESIZE + pg;

        ASSERT(pd->tables[pagenr / PAGE_COUNT]);
        ASSERT(changeAllowed(pd, pagenr / PAGE_COUNT));

        // Get the physical address and invalidate the page
        u32 *page = &pd->tables[pagenr / PAGE_COUNT]->pages[pagenr % PAGE_COUNT];
        *page = 0;

        if (pd->tables[pagenr / PAGE_COUNT] == currentPageDirectory->tables[pagenr / PAGE_COUNT])
            invalidateTLBEntry((void *) (pagenr * PAGESIZE));

    }
}

// ------------------------------------------------------------------------------
// Creates the mapping between a physical and a virtual address in the given page
// directory. Both vaddr and paddr should have been allocated previously.
// ------------------------------------------------------------------------------
int paging_mapVirtToPhysAddr(pageDirectory_t *pd, void *vaddr, u32 paddr, size_t pages, MEMFLAGS_t flags) {
    // "vaddr" must be page-aligned
    ASSERT(((u32) vaddr) % PAGESIZE == 0);

    for (u32 pg = 0; pg < pages; ++pg) {
        u32 pagenr = (u32) vaddr / PAGESIZE + pg;

        // Get the page table
        pageTable_t *pt = pd->tables[pagenr / PAGE_COUNT];

        // Is memory already allocated?
        if (!pt || !(pt->pages[pagenr % PAGE_COUNT] & MEM_ALLOCATED)) {
            printf("Page not allocated: %u\n", pagenr);
            return 0;
        }

        ASSERT(changeAllowed(pd, pagenr / PAGE_COUNT));

        pd->codes[pagenr / PAGE_COUNT] =
                getPhysAddr(pt) | MEM_PRESENT | MEM_WRITE | (flags & (~MEM_NOTLBUPDATE)); // Update codes

        // Setup the page
        pt->pages[pagenr % PAGE_COUNT] = (paddr + pg * PAGESIZE) | flags | MEM_PRESENT | MEM_ALLOCATED;

        if (pt == currentPageDirectory->tables[pagenr / PAGE_COUNT])
            invalidateTLBEntry(vaddr + pg * PAGESIZE);

#ifdef _DIAGNOSIS_
        if (flags & MEM_USER)
            printf("page %u now associated to physAddress %Xh\n", pagenr, paddr + pg * PAGESIZE);
#endif
    }
    return 1;
}


// ------------------------------------------------------------------------------
// Allocates memory of given size at given virtual address, allocates physical
// memory and creates mapping.
// ------------------------------------------------------------------------------
int paging_alloc(pageDirectory_t *pd, void *addr, u32 size, MEMFLAGS_t flags) {
    // "virtAddress" and "size" must be page-aligned
    ASSERT(((u32) addr) % PAGESIZE == 0);
    ASSERT(size % PAGESIZE == 0);

    size_t pages = size / PAGESIZE;

    // Allocate virtual memory
    if (!paging_allocVirtAddr(pd, addr, pages)) {
        printf("paging_allocVirtAddr(%X, %X, %u) failed.\n", pd, addr, pages);
        return 0;
    }

    // We repeat allocating one page at once
    for (size_t done = 0; done < pages; done++) {
        // Allocate physical memory
        u32 paddr = paging_allocPhysMem(1, 1);

        // Link physical with virtual memory
        if (!paging_mapVirtToPhysAddr(pd, addr + done * PAGESIZE, paddr, 1, flags)) {
            paging_freePhysMem(paddr, 1);
            printf("paging_mapVirtToPhysAddr(%X, %X, %X, %u, %X) failed.\n", pd, addr + done * PAGESIZE, paddr, 1, flags);
            return 0;
        }

#ifdef _DIAGNOSIS_
        if (flags & MEM_USER)
            printf("pagenumber now allocated: %u physAddress: %Xh\n", done, paddr);
#endif
    }
    return 1;
}

// ------------------------------------------------------------------------------
// Frees virtual and physical memory at given virtual address and deletes mapping.
// ------------------------------------------------------------------------------
void paging_free(pageDirectory_t *pd, void *virtAddress, u32 size) {
    // "virtAddress" and "size" must be page-aligned
    ASSERT(((u32) virtAddress) % PAGESIZE == 0);
    ASSERT(size % PAGESIZE == 0);

    // Go through all pages and free them
    size_t pagenr = (u32) virtAddress / PAGESIZE;

    while (size) {
        ASSERT(pd->tables[pagenr / PAGE_COUNT] && changeAllowed(pd, pagenr / PAGE_COUNT));

        // Get the physical address
        u32 physAddress = getPhysAddr(virtAddress);

        // Free virtual and physical memory
        paging_freeVirtMem(pd, virtAddress, 1);
        paging_freePhysMem(physAddress, 1);

        // Adjust variables for next loop run
        size -= PAGESIZE;
        pagenr++;
    }
}

// ------------------------------------------------------------------------------
// Sets the flags for a given area of memory in a given page directory.
// ------------------------------------------------------------------------------
void paging_setFlags(pageDirectory_t *pd, void *virtAddress, u32 size, MEMFLAGS_t flags) {
    // "virtAddress" and "size" must be page-aligned
    ASSERT(((u32) virtAddress) % PAGESIZE == 0);
    ASSERT(size % PAGESIZE == 0);

    // Check whether a page is allocated within the area
    for (u32 done = 0; done < size / PAGESIZE; done++) {
        u32 pagenr = (u32) virtAddress / PAGESIZE + done;
        ASSERT(pd->tables[pagenr / PAGE_COUNT] &&
               (pd->tables[pagenr / PAGE_COUNT]->pages[pagenr % PAGE_COUNT] & MEM_ALLOCATED) &&
               changeAllowed(pd, pagenr / PAGE_COUNT));

        u32 *page = &pd->tables[pagenr / PAGE_COUNT]->pages[pagenr % PAGE_COUNT];
        *page = (*page & 0xFFFFF000) | flags | MEM_PRESENT | MEM_ALLOCATED;

        if (pd->tables[pagenr / PAGE_COUNT] == currentPageDirectory->tables[pagenr / PAGE_COUNT])
            invalidateTLBEntry((void *) (pagenr * PAGESIZE));
    }
}

// ------------------------------------------------------------------------------
// Destroys given page directory and frees all physical memory associated with it.
// ------------------------------------------------------------------------------
pageDirectory_t *paging_createPageDirectory(void) {
    // Allocate memory for the page directory
    pageDirectory_t *pd = (pageDirectory_t *) malloc(sizeof(pageDirectory_t), PAGESIZE, "pageDirectory_t");

    if (!pd)
        return (0);

    // Each user's page directory contains the same mapping as the kernel
    memcpy(pd, kernelPageDirectory, sizeof(pageDirectory_t));
    pd->physAddr = getPhysAddr(pd->codes);

    return (pd);
}

// ------------------------------------------------------------------------------
// Destroys given page directory and frees all physical memory associated with it.
// ------------------------------------------------------------------------------
void paging_destroyPageDirectory(pageDirectory_t *pd) {
    ASSERT(pd != kernelPageDirectory); // The kernel's page directory must not be destroyed
    if (pd == currentPageDirectory)
        paging_switch(kernelPageDirectory); // Leave current PD, if we attempt to delete it

    // Free all memory that is not from the kernel
    for (u32 i = 0; i < PT_COUNT; i++) {
        if (pd->tables[i] && changeAllowed(pd, i)) {
            for (u32 j = 0; j < PAGE_COUNT; j++) {
                u32 physAddress = pd->tables[i]->pages[j] & 0xFFFFF000;

                if (physAddress) {
                    paging_freePhysMem(physAddress, 1);
                }
            }
            free(pd->tables[i]);
        }
    }

    free(pd);
}

// ------------------------------------------------------------------------------
// Allocates physical and virtual memory with given physical address, as commonly
// required for MMIO.
// ------------------------------------------------------------------------------
void *paging_allocMMIO(u32 paddr, size_t pages) {
    static void *head = (void *) PCI_MEM_START;

    if (!paging_allocPhysAddr(paddr, pages)) {
        // TODO: Enable this.
        //printfe("\nPhysical address not available for MMIO.");
        //return (0);
    }

    void *vaddr = head;
    head += pages * PAGESIZE;

    // Allocate virtual memory
    // TODO: Use paging_allocVirtMem instead of paging_allocVirtAddr to get rid of PCI_MEM-area
    if (!paging_allocVirtAddr(currentPageDirectory, vaddr, pages)) {
        printf("\nNot enough virtual memory for MMIO available.\n");
        return (0);
    }
    paging_mapVirtToPhysAddr(currentPageDirectory, vaddr, paddr, pages, MEM_WRITE);

    return vaddr;
}

// ------------------------------------------------------------------------------
// Calculates physical address the virtual address is mapped to in current pd.
// ------------------------------------------------------------------------------
u32 getPhysAddr(const void *virtAddress) {
    // Find the page table
    u32 pageNumber = (u32) virtAddress / PAGESIZE;
    pageTable_t *pt = currentPageDirectory->tables[pageNumber / PAGE_COUNT];

    printf("\nvirt-->phys: pagenr: %u, pt: %Xh\n", pageNumber, pt);

    if (pt) {
        // Read address, cut off flags, append odd part of virtual address
        return ((pt->pages[pageNumber % PAGE_COUNT] & 0xFFFFF000) + ((u32) virtAddress & 0x00000FFF));
    } else {
        return (0); // not mapped
    }
}