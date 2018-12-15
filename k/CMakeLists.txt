include_directories(.)
include_directories(include)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/k.lds")

set(K_FILES
        sys/allocator.c sys/allocator.h
        elf.h elf.c
        include/compiler.h include/cpu.h io/io.h include/multiboot.h
        sys/gdt.c sys/gdt.h
        sys/idt.c sys/idt.h
        sys/interrupt-handler.c
        k.c
        io/keyboard.c io/keyboard.h
        io/kfilesystem.c io/kfilesystem.h
        io/libvga.c io/libvga.h
        sys/paging.c sys/paging.h
        sys/physical-memory.c sys/physical-memory.h
        io/pic.c io/pic.h
        io/pit.c io/pit.h
        io/serial.c io/serial.h
        sys/syscall.c sys/syscall.h
        task.c task.h
        io/terminal.c io/terminal.h
        io/filesystem.c io/filesystem.h
        sheduler.c sheduler.h
        utils/list.c include/list.h)

set(K_LIBS
        utils/memcmp.c
        utils/memcpy.c
        utils/memset.c
        utils/kprintf.c
        utils/kputs.c
        utils/strcmp.c
        utils/strcpy.c
        utils/strlen.c
        utils/strncmp.c
        utils/strnlen.c utils/strncpy.c utils/memmove.c utils/strdup.c)

set_property(SOURCE crt0.S PROPERTY LANGUAGE C)
set_property(SOURCE sys/interrupt-entry.S PROPERTY LANGUAGE C)

set(K_ASM
        crt0.S
        sys/interrupt-entry.S)

add_executable(kernel_epita ${K_FILES} ${K_ASM} ${K_LIBS})