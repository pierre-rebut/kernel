isr_handle:
    pusha
    pushl %esp
    call interrupt_handler
    add $4, %esp
    popa
    add $8, %esp
    iret

.macro isr_macro id
    .global isr_handle\id
isr_handle\id:
    pushl $0
    pushl $\id
    jmp isr_handle
.endm

.macro isr_error_macro id
    .global isr_handle\id
isr_handle\id:
    pushl $\id
    jmp isr_handle
.endm

// ISR handler
isr_macro 0
isr_macro 1
isr_macro 2
isr_macro 3
isr_macro 4
isr_macro 5
isr_macro 6
isr_macro 7
isr_error_macro 8
isr_macro 9
isr_error_macro 10
isr_error_macro 11
isr_error_macro 12
isr_error_macro 13
isr_error_macro 14
isr_macro 15
isr_macro 16
isr_error_macro 17
isr_macro 18
isr_macro 19
isr_macro 20
isr_macro 21
isr_macro 22
isr_macro 23
isr_macro 24
isr_macro 25
isr_macro 26
isr_macro 27
isr_macro 28
isr_macro 29
isr_macro 30
isr_macro 31

// ISQ handler
isr_macro 32
isr_macro 33