//
// Created by rebut_p on 21/09/18.
//

#include "idt.h"

struct idt_entry idt[256] = {0};

static void addEntry(struct idt_entry *entry, void (*handle)(), u8 interruptType, u8 dpl)
{
    entry->offset = (u16) (((u32) handle >> 0) & 0xFFFF);
    entry->offset2 = (u16) (((u32) handle >> 16) & 0xFFFF);
    entry->segmentSelector = 0x08;
    entry->reserved = 0;
    entry->present = 1;
    entry->d = interruptType;
    entry->dpl = dpl;
}

static void initIdt()
{
    struct idt_r
    {
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

void initInterrupt()
{

#define ISR(id, type, dpl) \
    void isr_handle##id(); \
    addEntry(&idt[id], isr_handle##id, type, dpl);

#define ISR_ERROR(id, type, dpl) \
    void isr_handle##id(); \
    addEntry(&idt[id], isr_handle##id, type, dpl);

#include "../macros_isr.def"

#undef ISR
#undef ISR_ERROR

    initIdt();
}