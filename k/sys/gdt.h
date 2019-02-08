//
// Created by rebut_p on 21/09/18.
//

#ifndef KERNEL_EPITA_GDT_H
#define KERNEL_EPITA_GDT_H

#include <k/types.h>
#include <compiler.h>
#include <multiboot.h>

void initMemory();
void switchTSS(u32 esp0, u32 esp, u32 ss);

struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    u8 access : 7;
    u8 present : 1;
    u8 granularity;
    u8 base_high;
} __attribute__((packed));

struct tss_entry {
    u32 prev_tss;
    u32 esp0;
    u32 ss0;
    u32 esp1;
    u32 ss1;
    u32 esp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldt;
    u16 trap;
    u16 iomap_base;
} __attribute__((packed));

#endif //KERNEL_EPITA_GDT_H
