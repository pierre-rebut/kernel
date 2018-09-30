#ifndef KHEAP_H
#define KHEAP_H

#include <k/types.h>

#define HEAP_ALIGNMENT_MASK 0x00FFFFFF
#define HEAP_WITHIN_PAGE (1 << 24)
#define HEAP_WITHIN_64K (1 << 25)
#define HEAP_CONTINUOUS (1 << 31)

void *heap_getCurrentEnd(void);
void heap_install(void);
void heap_logRegions(void);

void *kmalloc(size_t size, u32 alignment);
void kfree(void *addr);


#endif
