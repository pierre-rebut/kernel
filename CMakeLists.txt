cmake_minimum_required(VERSION 3.8)
project(kernel_epita)

include_directories(k/include)

set(K_FILES
        k/compiler.h
        k/io/io.h
        k/multiboot.h
        k/elf.h
        k/io/serial.c
        k/sys/gdt.c
        k/k.c
        k/include/k/kstd.h k/include/k/kfs.h k/include/k/types.h
        k/io/libvga.c k/sys/idt.c k/sys/idt.h k/io/pic.c k/io/pic.h k/interrupt-handler.c k/io/keyboard.c k/io/keyboard.h k/io/pit.c k/io/pit.h k/sys/syscall.c k/sys/syscall.h k/kfilesystem.c k/kfilesystem.h k/terminal.c k/terminal.h k/task.c k/task.h k/binary.c k/binary.h k/io/mouse.c k/io/mouse.h k/sys/paging.c k/sys/paging.h k/include/utils.h k/cpu.h k/sys/kalloc.c k/sys/kalloc.h k/utils/error.c k/include/error.h k/sys/paging2.c k/sys/paging2.h)

add_executable(kernel_epita ${K_FILES})