//
// Created by rebut_p on 21/09/18.
//

#ifndef KERNEL_EPITA_INTERRUPT_H
#define KERNEL_EPITA_INTERRUPT_H

#include <k/types.h>

void initInterupt();

// ISR handler
void isr_handle0();
void isr_handle1();
void isr_handle2();
void isr_handle3();
void isr_handle4();
void isr_handle5();
void isr_handle6();
void isr_handle7();
void isr_handle8();
void isr_handle9();
void isr_handle10();
void isr_handle11();
void isr_handle12();
void isr_handle13();
void isr_handle14();
void isr_handle15();
void isr_handle16();
void isr_handle17();
void isr_handle18();
void isr_handle19();
void isr_handle20();
void isr_handle21();
void isr_handle22();
void isr_handle23();
void isr_handle24();
void isr_handle25();
void isr_handle26();
void isr_handle27();
void isr_handle28();
void isr_handle29();
void isr_handle30();
void isr_handle31();

// IQR Handler
void isr_handle32();
void isr_handle33();

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
