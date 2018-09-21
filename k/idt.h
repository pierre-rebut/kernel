//
// Created by rebut_p on 21/09/18.
//

#ifndef KERNEL_EPITA_INTERRUPT_H
#define KERNEL_EPITA_INTERRUPT_H

#include <k/types.h>

void initInterupt();

// List isr handler
void isr_handle_divide_zero();
void isr_handle_keyboard();

struct idt_entry {
    u16 offset;
    u16 segmentSelector;
    u8 reserved;
    u8 d : 5;
    u8 dpl : 2;
    u8 present : 1;
    u16 offset2;
}__attribute__((packed));

struct idt_context {
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 int_no;
    u32 err_code;
};

#endif //KERNEL_EPITA_INTERRUPT_H
