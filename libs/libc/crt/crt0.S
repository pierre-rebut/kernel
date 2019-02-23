        .section .text

        .global _start
_start:
        # Set up end of the stack frame linked list.
        mov $0, %ebp
        push %ebp # eip=0
        push %ebp # ebp=0
        mov %esp, %ebp

        # We need those in a moment when we call main.
        push %edx
        push %ecx
        push %eax

        # Run the global constructors.
        call _init

        # Run main
        call main

        # Terminate the process with the exit code.
        push %eax

        call _fini
        call exit
        .size _start, . - _start
