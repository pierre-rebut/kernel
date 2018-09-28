isr_handle:
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %ebp
    pushl %esi
    pushl %edi

    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl %esp
    call interrupt_handler
    movl %eax, %esp

    popl %gs
    popl %fs
    popl %es
    popl %ds

    popl %edi
    popl %esi
    popl %ebp
    popl %ebx
    popl %edx
    popl %ecx
    popl %eax

breakpointhere:

    add $8, %esp
    iret

#define ISR(id, type, dpl) \
    .global isr_handle##id ;\
isr_handle##id: ;\
    pushl $0 ;\
    pushl $id;\
    jmp isr_handle ;

#define ISR_ERROR(id, type, dpl) \
    .global isr_handle##id ;\
isr_handle##id: ;\
    pushl $id;\
    jmp isr_handle ;

#include "macros_isr.def"

#undef ISR
#undef ISR_ERROR

