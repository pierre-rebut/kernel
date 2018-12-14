//
// Created by rebut_p on 15/11/18.
//

#ifndef KERNEL_EPITA_ALLOCATOR_H
#define KERNEL_EPITA_ALLOCATOR_H

#include <k/types.h>
#include "../include/multiboot.h"

#define KERNEL_HEAP_START 0xE0000000
#define PLACEMENT_BEGIN   0x600000
#define PLACEMENT_END     0xC00000

#define HEAP_ALIGNMENT_MASK 0x00FFFFFF
#define HEAP_WITHIN_PAGE 1 << 24
#define HEAP_WITHIN_64K 1 << 25
#define HEAP_CONTINUOUS 1 << 31

typedef struct
{
    u32 size;
    u32 number;
    char     reserved;
} __attribute__((packed)) region_t;

int initAllocator();
void *kmalloc(u32 size, u32 allign);
void kfree(void *alloc);

#endif //KERNEL_EPITA_ALLOCATOR_H
