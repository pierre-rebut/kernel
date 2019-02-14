.global entry
entry:
    push %edx
    push %ecx
    push %eax

    call _init
    call main
    call exit