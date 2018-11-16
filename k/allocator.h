//
// Created by rebut_p on 15/11/18.
//

#ifndef KERNEL_EPITA_ALLOCATOR_H
#define KERNEL_EPITA_ALLOCATOR_H

#include <k/types.h>
#include "multiboot.h"

#define TMP_MAX_SIZE 0xC00000
#define KERNEL_HEAP_START 0xE0000000

#define ALLOC_MAGIC_NB 42

enum Allocator {
    MALLOC,
    FREE
};

struct Region {
    int magic_nb;
    u32 size;
    struct Region *next;
    struct Region *prev;
    enum Allocator status;
} __attribute__((packed));

void initTemporaryAllocator(const multiboot_info_t *info);
int initAllocator();
void *kmalloc(u32 size, u32 allign);
void kfree(void *alloc);

#endif //KERNEL_EPITA_ALLOCATOR_H
