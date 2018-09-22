isr_handle:
    pusha
    pushl %esp
    call interrupt_handler
    add $4, %esp
    popa
    add $8, %esp
    iret

#define ISR(id, type) \
.global isr_handle##id ;\
isr_handle##id: ;\
pushl $0 ;\
pushl $id;\
jmp isr_handle ;

#define ISR_ERROR(id, type) \
.global isr_handle##id ;\
isr_handle##id: ;\
pushl $id;\
jmp isr_handle ;

#include "macros_isr.def"

#undef ISR
#undef ISR_ERROR

