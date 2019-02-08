.global entry
entry:
    push %edx
    push %ecx
    push %eax

    call main
    call exit