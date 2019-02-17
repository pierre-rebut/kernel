//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_ALLOC_H
#define KERNEL_ALLOC_H

#include <stddef.h>

int getpagesize();

void *malloc(size_t len);

void *realloc(void *ptr, size_t len);

void *calloc(size_t len, size_t);

void free(const void *ptr);

void show_alloc_mem();

#endif //KERNEL_ALLOC_H
