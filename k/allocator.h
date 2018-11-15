//
// Created by rebut_p on 15/11/18.
//

#ifndef KERNEL_EPITA_ALLOCATOR_H
#define KERNEL_EPITA_ALLOCATOR_H

#include "multiboot.h"

#define TMP_MAX_SIZE 0xC00000
#define KERNEL_HEAP_START 0xE0000000

void initTemporaryAllocator(const multiboot_info_t *info);
void initAllocator();

void *kmalloc(u32 size, u32 allign);
void kfree(void *alloc);

#endif //KERNEL_EPITA_ALLOCATOR_H
