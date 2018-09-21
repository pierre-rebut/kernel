//
// Created by rebut_p on 21/09/18.
//

#ifndef KERNEL_EPITA_GDT_H
#define KERNEL_EPITA_GDT_H
#include <k/types.h>

void initMemory();

struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    u8 access : 7;
    u8 present : 1;
    u8 limit_mid : 4;
    u8 avl : 1;
    u8 l : 1;
    u8 granularity : 2;
    u8 base_hight;
} __attribute__((packed));

#endif //KERNEL_EPITA_GDT_H
