cmake_minimum_required(VERSION 3.8)
project(kernel_epita)

include_directories(k/include)

set(K_FILES
        k/compiler.h
        k/io.h
        k/multiboot.h
        k/elf.h
        k/serial.c
        k/gdt.c
        k/k.c
        k/libvga.c k/idt.c k/idt.h k/pic.c k/pic.h k/interrupt-handler.c k/getkey.c k/getkey.h k/pit.c k/pit.h k/syscall.c k/syscall.h)

add_executable(kernel_epita ${K_FILES})