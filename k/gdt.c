#include "gdt.h"
#include "compiler.h"

/* struct gdt_entry gdt[] = {
        {0,      0, 0, 0,    0,    0},          // NULL entry
        {0xFFFF, 0, 0, 0x9A, 0xCF, 0},          // Kernel code entry
        {0xFFFF, 0, 0, 0x92, 0xCF, 0},          // Kernel data entry
        {0xFFFF, 0, 0, 0xFA, 0,    0},          // User code entry
        {0xFFFF, 0, 0, 0xF2, 0,    0},          // User data entry
        {0x68,   0, 0, 0xE9, 0,    0}           // Task state segment entry
};*/

struct gdt_entry gdt[] = {
        // Null Entry
        {0},
        // Kernel code entry
        {
                .limit_low = 0xFFFF,
                .access = (0 << 5) | (1 << 4) | 0xA,
                .present = 1,
                .limit_mid = 0xF,
                .granularity = 3,
        },
        // Kernel data entry
        {
                .limit_low = 0xFFFF,
                .access = (0 << 5) | (1 << 4) | 0x3,
                .present = 1,
                .limit_mid = 0xF,
                .granularity = 3,
        },
        // User code entry
        {
                .limit_low = 0xFFFF,
                .access = (3 << 5) | (1 << 4) | 0xA,
                .present = 1
        },
        // User data entry
        {
                .limit_low = 0xFFFF,
                .access = (3 << 5) | (1 << 4) | 0x3,
                .present = 1
        },
        // TSS entry
        {
                .limit_low = 0x68,
                .access = (3 << 5) | (0 << 4) | 0x9,
                .present = 1,
        }
};

static void initGdt() {
    struct gdt_r {
        u16 limit;
        u32 base;
    }__packed;

    struct gdt_r gdtr;

    gdtr.base = (u32) gdt;
    gdtr.limit = sizeof(gdt) - 1;

    asm volatile("lgdt %0\n"
    : /* no output */
    : "m" (gdtr)
    : "memory");
}

static void initProtectedMode() {
    asm volatile("movl %cr0, %eax\n"
            "or %eax, 1\n"
            "movl %eax, %cr0\n");
}

static void setDataSegment() {

    // set ds, fs, gs, ss
    asm volatile("movw %w0, %%ds\n"
            "movw %w0, %%es\n"
            "movw %w0, %%fs\n"
            "movw %w0, %%gs\n"
            "movw %w0, %%ss\n"
    :
    : "a" (0x10));

    // set cs
    asm volatile("ljmp $0x08, $1f\n"
            "1:");
}

static void initTaskSegment() {
    asm volatile("movw $0x28, %ax\n"
            "ltr %ax\n");
}

void initMemory() {
    initGdt();
    initTaskSegment();
    initProtectedMode();
    setDataSegment();
}