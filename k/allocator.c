//
// Created by rebut_p on 15/11/18.
//

#include <k/types.h>
#include "allocator.h"
#include "physical-memory.h"

static u32 isTemporaryAllocator;
static void *temporaryAllocAddr;

void initTemporaryAllocator(const multiboot_info_t *info) {
    isTemporaryAllocator = 1;
    temporaryAllocAddr = (void*) ((module_t *)info->mods_addr)->mod_end;
}

void initAllocator() {
    isTemporaryAllocator = 0;
}

static void *temporaryKMalloc(u32 size, u32 allign) {
    size = alignUp(size, 4);

    void *currPlacement = (void *) alignUp((u32) temporaryAllocAddr, allign);
    if ((u32) currPlacement + size > TMP_MAX_SIZE)
        return 0;

    temporaryAllocAddr = currPlacement + size;
    return currPlacement;
}

void *kmalloc(u32 size, u32 allign) {
    allign &= 0x00FFFFFF;

    if (isTemporaryAllocator)
        return temporaryKMalloc(size, allign);

    return NULL;
}

void kfree(void *alloc) {

}