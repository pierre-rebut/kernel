#include "gdt.h"
#include "compiler.h"

struct tss_entry tss = {
        .ss0 = 0x10, // Kernel Data Segment
        .esp0 = 0,
        .es = 0x10, // Kernel Data Segment
        .cs = 0x08, // Kernel Code Segment
        .ds = 0x10, // Kernel Data Segment
        .fs = 0x10, // Kernel Data Segment
        .gs = 0x10, // Kernel Data Segment
};

struct gdt_entry gdt[6] = {};

void addGdtEntry(u32 id, u32 base, u32 limit, u8 access, u8 gran) {
    gdt[id].base_low = (u16) (base & 0xFFFF);
    gdt[id].base_mid = (u8) ((base >> 16) & 0xFF);
    gdt[id].base_high = (u8) ((base >> 24) & 0xFF);
    gdt[id].limit_low = (u16) (limit & 0xFFFF);
    gdt[id].granularity = (u8) ((limit >> 16) & 0x0F);
    gdt[id].granularity |= (gran & 0xF0);
    gdt[id].access = access;
    gdt[id].present = 1;
}

static void initGdt() {
    addGdtEntry(0, 0, 0, 0, 0); // NULL segment
    addGdtEntry(1, 0, 0xFFFFFFF, (0 << 5) | (1 << 4) | 0xA, 0xCF); // KERNEL code segment
    addGdtEntry(2, 0, 0xFFFFFFF, (0 << 5) | (1 << 4) | 0x3, 0xCF); // KERNEL data segment
    addGdtEntry(3, 0, 0xFFFFFFF, (3 << 5) | (1 << 4) | 0xA, 0xCF); // USER code segment
    addGdtEntry(4, 0, 0xFFFFFFF, (3 << 5) | (1 << 4) | 0x3, 0xCF); // USER data segment
    addGdtEntry(5, (u32) &tss, sizeof(tss), 0xE9, 0); // TSS

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
    initProtectedMode();
    setDataSegment();
    initTaskSegment();
}

void switchTSS(u32 esp0, u32 esp, u32 ss) {
    tss.esp0 = esp0;
    tss.esp = esp;
    tss.ss = ss;
}