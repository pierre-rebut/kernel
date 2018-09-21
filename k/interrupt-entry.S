isr_handle:
    pusha
    pushl %esp
    call interrupt_handler
    add $4, %esp
    popa
    add $8, %esp
    iret

.global isr_handle_divide_zero
.type isr_handle_divide_zero, @function
isr_handle_divide_zero:
    pushl $0
    pushl $0
    jmp isr_handle

.global isr_handle_keyboard
.type isr_handle_keyboard, @function
isr_handle_keyboard:
    pushl $0
    pushl $33
    jmp isr_handle