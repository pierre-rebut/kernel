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
    addEntry(&idt[0], isr_handle_divide_zero, IDT_TYPE_INTERRUPT);
    addEntry(&idt[33], isr_handle_keyboard, IDT_TYPE_INTERRUPT);

    initIdt();
}