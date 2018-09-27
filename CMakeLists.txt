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
        k/include/k/kstd.h k/include/k/kfs.h k/include/k/types.h
        k/libvga.c k/idt.c k/idt.h k/pic.c k/pic.h k/interrupt-handler.c k/getkey.c k/getkey.h k/pit.c k/pit.h k/syscall.c k/syscall.h k/kfilesystem.c k/kfilesystem.h k/terminal.c k/terminal.h k/userland.c k/userland.h k/binary.c k/binary.h)

add_executable(kernel_epita ${K_FILES})