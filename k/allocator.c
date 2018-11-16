//
// Created by rebut_p on 15/11/18.
//

#include <k/types.h>
#include <stdio.h>
#include "allocator.h"
#include "physical-memory.h"
#include "paging.h"

static void *temporaryAllocAddr;
static struct Region *region = NULL;
static u32 nbRegions = 0;

void initTemporaryAllocator(const multiboot_info_t *info) {
    temporaryAllocAddr = (void *) ((module_t *) info->mods_addr)->mod_end;
}

static void *
create_header(struct Region *ptr, u32 len, struct Region *next, struct Region *prev, enum Allocator status) {
    if (ptr == NULL)
        return (NULL);

    ptr->magic_nb = ALLOC_MAGIC_NB;
    ptr->size = len;
    ptr->next = next;
    ptr->prev = prev;
    ptr->status = status;
    return (void *) ptr;
}

int initAllocator() {
    if (pagingAlloc(kernelPageDirectory, (void *) KERNEL_HEAP_START, PAGESIZE, MEM_WRITE)) {
        printf("Init allocator failed\n");
        return -1;
    }

    region = (void *) KERNEL_HEAP_START;
    nbRegions += 1;

    create_header(region, PAGESIZE - sizeof(struct Region), NULL, NULL, FREE);
    return 0;
}

static void *temporaryKMalloc(u32 size, u32 allign) {
    void *currPlacement = (void *) alignUp((u32) temporaryAllocAddr, allign);
    if ((u32) currPlacement + size > TMP_MAX_SIZE)
        return 0;

    temporaryAllocAddr = currPlacement + size;
    return currPlacement;
}

static void *malloc_new_pagesize(u32 len, struct Region *header) {
    u32 tmp = 0;
    u32 toto = 0;

    if (header->status == FREE)
        toto += header->size;

    while (toto < (len + sizeof(struct Region))) {
        toto += PAGESIZE;
        tmp++;
    }

    if (tmp != 0) {
        printf("here i am\n");
        if (pagingAlloc(kernelPageDirectory, (void *) KERNEL_HEAP_START + (nbRegions * PAGESIZE), PAGESIZE * tmp,
                        MEM_WRITE))
            return NULL;

        nbRegions += tmp;
    }

    if (header->status == FREE) {
        toto -= len;
        header = create_header(header, len, NULL, header->prev, MALLOC);
        header->next = create_header((void *) header + sizeof(struct Region) + len, toto, NULL, header, FREE);
        return (void *) header + sizeof(struct Region);
    }

    struct Region *new = create_header((void *) header + sizeof(struct Region) + header->size, len, NULL, header,
                                       MALLOC);
    header->next = new;

    toto -= len + sizeof(struct Region);
    new->next = create_header((void *) new + sizeof(struct Region) + len, toto, NULL, new, FREE);

    return (void *) new + sizeof(struct Region);
}

static void *malloc_reallocsize(struct Region *header, u32 len, u32 allign) {
    struct Region *new;
    struct Region *link;

    u32 tmp = header->size;
    header = create_header((void*) alignUp((u32) header + sizeof(struct Region), allign) - sizeof(struct Region),
                           len, header->next, header->prev, MALLOC);

    if (tmp - len >= sizeof(struct Region)) {
        link = header->next;
        new = (struct Region *) create_header((void *) header + len + sizeof(struct Region),
                                              tmp - len - sizeof(struct Region),
                                              link, header, FREE);
        if (link != NULL)
            link->prev = new;
        header->next = new;
    }
    return ((void *) header + sizeof(struct Region));
}

void *kmalloc(u32 size, u32 allign) {
    if (size == 0)
        return (NULL);

    allign &= 0x00FFFFFF;
    size = alignUp(size, 4);

    if (!region)
        return temporaryKMalloc(size, allign);

    printf("kmalloc: %u\n", size);

    struct Region *iter = region;
    struct Region *prev = NULL;

    while (iter->next != NULL)
        iter = iter->next;

    u32 tmp = 0;
    u32 toto = 0;

    if (iter->status == FREE)
        toto += iter->size;

    while (toto < (size + sizeof(struct Region))) {
        toto += PAGESIZE;
        tmp++;
    }

    if (tmp != 0) {
        printf("here i am\n");
        if (pagingAlloc(kernelPageDirectory, (void *) KERNEL_HEAP_START + (nbRegions * PAGESIZE), PAGESIZE * tmp,
                        MEM_WRITE))
            return NULL;

        nbRegions += tmp;
    }

    struct Region *tmp = (void*) alignUp((u32)iter + iter->size + sizeof(struct Region), PAGESIZE) - sizeof(struct Region);

    tmp = create_header(tmp, size, NULL, iter, MALLOC);
    tmp->next = create_header((void*) tmp + iter->size, len, NULL, tmp, FREE);


    while (iter != NULL) {
        if (iter->magic_nb != ALLOC_MAGIC_NB) {
            printf("exit 1\n");
            return (NULL);
        }

        if (iter->status == FREE && iter->size >= size) {
            printf("exit 2\n");
            return (malloc_reallocsize(iter, size));
        }

        prev = iter;
        iter = iter->next;
    }

    if (prev == NULL) {
        return NULL;
    }

    return malloc_new_pagesize(size, prev);
}

void kfree(void *alloc) {
    struct Region *header = alloc - sizeof(struct Region);
    if (header->magic_nb != ALLOC_MAGIC_NB || header->status == FREE)
        return;

    header->status = FREE;
}