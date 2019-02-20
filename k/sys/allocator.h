//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_ALLOCATOR2_H
#define KERNEL_ALLOCATOR2_H

#include <k/ktypes.h>
#include <multiboot.h>

#define KERNEL_HEAP_START 0xE0000000
#define PLACEMENT_BEGIN   0x600000
#define PLACEMENT_END     0xC00000

#define HEAP_ALIGNMENT_MASK 0x00FFFFFF
#define HEAP_WITHIN_PAGE 1 << 24
#define HEAP_WITHIN_64K 1 << 25
#define HEAP_CONTINUOUS 1 << 31

#define USER_STACK          0x1500000
#define USER_ARG_BUFFER     USER_STACK
#define USER_ENV_BUFFER     (USER_STACK + 0x1000)
#define USER_HEAP_START     (USER_STACK + 0x10000)
#define KERNEL_STACK_SIZE 0x1000

int initAllocator();

void *kmalloc(u32 size, u32 allign, const char *comment);

void kfree(void *alloc);

void kmallocGetInfo(u32 *total, u32 *used);

#endif //KERNEL_ALLOCATOR2_H
