//
// Created by rebut_p on 21/09/18.
//

#ifndef KERNEL_EPITA_INTERRUPT_H
#define KERNEL_EPITA_INTERRUPT_H

#include <k/ktypes.h>
#include <compiler.h>

struct idt_entry
{
    u16 offset;
    u16 segmentSelector;
    u8 reserved;
    u8 d : 5;
    u8 dpl : 2;
    u8 present : 1;
    u16 offset2;
}__attribute__((packed));

struct esp_context
{
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, ebx, edx, ecx, eax;
    u32 int_no, err_code;
    u32 eip, cs, eflags, useresp, ss;
};

void initInterrupt();

int interruptRegister(u32 int_no, void (*fct)(struct esp_context *));

#endif //KERNEL_EPITA_INTERRUPT_H
