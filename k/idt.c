//
// Created by rebut_p on 21/09/18.
//

#include "idt.h"
#include "compiler.h"

#define IDT_TYPE_INTERRUPT 0xE
#define IDT_TYPE_TRAP 0xF

struct idt_entry idt[256] = {0};

static void addEntry(struct idt_entry *entry, void (*handle)(), u8 interruptType) {
    entry->offset = (u16)(((u32)handle >> 0) & 0xFFFF);
    entry->offset2 = (u16)(((u32)handle >> 16) & 0xFFFF);
    entry->segmentSelector = 0x08;
    entry->reserved = 0;
    entry->present = 1;
    entry->d = interruptType;
}

static void initIdt() {
    struct idt_r {
        u16 limit;
        u32 base;
    }__packed;

    struct idt_r idtr;
    idtr.base = (u32) idt;
    idtr.limit = sizeof(idt) - 1;
    asm volatile("lidt %0\n"
    : /* no output */
    : "m" (idtr)
    : "memory");
}

void initInterupt() {
    // ISR handler
    addEntry(&idt[0], isr_handle0, IDT_TYPE_INTERRUPT);
    addEntry(&idt[1], isr_handle1, IDT_TYPE_INTERRUPT);
    addEntry(&idt[2], isr_handle2, IDT_TYPE_INTERRUPT);
    addEntry(&idt[3], isr_handle3, IDT_TYPE_INTERRUPT);
    addEntry(&idt[4], isr_handle4, IDT_TYPE_INTERRUPT);
    addEntry(&idt[5], isr_handle5, IDT_TYPE_INTERRUPT);
    addEntry(&idt[6], isr_handle6, IDT_TYPE_INTERRUPT);
    addEntry(&idt[7], isr_handle7, IDT_TYPE_INTERRUPT);
    addEntry(&idt[8], isr_handle8, IDT_TYPE_INTERRUPT);
    addEntry(&idt[9], isr_handle9, IDT_TYPE_INTERRUPT);
    addEntry(&idt[10], isr_handle10, IDT_TYPE_INTERRUPT);
    addEntry(&idt[11], isr_handle11, IDT_TYPE_INTERRUPT);
    addEntry(&idt[12], isr_handle12, IDT_TYPE_INTERRUPT);
    addEntry(&idt[13], isr_handle13, IDT_TYPE_INTERRUPT);
    addEntry(&idt[14], isr_handle14, IDT_TYPE_INTERRUPT);
    addEntry(&idt[15], isr_handle15, IDT_TYPE_INTERRUPT);
    addEntry(&idt[16], isr_handle16, IDT_TYPE_INTERRUPT);
    addEntry(&idt[17], isr_handle17, IDT_TYPE_INTERRUPT);
    addEntry(&idt[18], isr_handle18, IDT_TYPE_INTERRUPT);
    addEntry(&idt[19], isr_handle19, IDT_TYPE_INTERRUPT);
    addEntry(&idt[20], isr_handle20, IDT_TYPE_INTERRUPT);
    addEntry(&idt[21], isr_handle21, IDT_TYPE_INTERRUPT);
    addEntry(&idt[22], isr_handle22, IDT_TYPE_INTERRUPT);
    addEntry(&idt[23], isr_handle23, IDT_TYPE_INTERRUPT);
    addEntry(&idt[24], isr_handle24, IDT_TYPE_INTERRUPT);
    addEntry(&idt[25], isr_handle25, IDT_TYPE_INTERRUPT);
    addEntry(&idt[26], isr_handle26, IDT_TYPE_INTERRUPT);
    addEntry(&idt[27], isr_handle27, IDT_TYPE_INTERRUPT);
    addEntry(&idt[28], isr_handle28, IDT_TYPE_INTERRUPT);
    addEntry(&idt[29], isr_handle29, IDT_TYPE_INTERRUPT);
    addEntry(&idt[30], isr_handle30, IDT_TYPE_INTERRUPT);
    addEntry(&idt[31], isr_handle31, IDT_TYPE_INTERRUPT);

    // ISQ handler
    addEntry(&idt[32], isr_handle32, IDT_TYPE_INTERRUPT);
    addEntry(&idt[33], isr_handle33, IDT_TYPE_INTERRUPT);

    initIdt();
}