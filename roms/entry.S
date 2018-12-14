extern main
extern exit

global entry
.entry:
    push %ecx
    push %eax

    call main
    call exit