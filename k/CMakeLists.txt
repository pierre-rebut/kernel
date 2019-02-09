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
        io/fs/kfilesystem.c io/fs/kfilesystem.h
        io/libvga.c io/libvga.h
        sys/paging.c sys/paging.h
        sys/physical-memory.c sys/physical-memory.h
        io/pic.c io/pic.h
        io/pit.c io/pit.h
        io/serial.c io/serial.h
        sys/syscall.c sys/syscall.h
        task.c task.h
        io/terminal.c io/terminal.h
        io/fs/filesystem.c io/fs/filesystem.h
        sheduler.c sheduler.h
        utils/list.c include/list.h
        io/device/ata.c io/device/ata.h
        sys/console.c sys/console.h
        sys/kobject.c sys/kobject.h
        io/fs/procfilesystem.c io/fs/procfilesystem.h
        io/cmos.c io/cmos.h
        sys/time.c sys/time.h sys/mutex.c sys/mutex.h io/device/device.c io/device/device.h io/fs/devfilesystem.c io/fs/devfilesystem.h io/fs/ext2filesystem.c io/fs/ext2filesystem.h io/device/fscache.c io/device/fscache.h io/pipe.c io/pipe.h)

set(K_LIBS
        utils/kprintf.c
        utils/kputs.c
        utils/memmove.c utils/string.c utils/mem.c utils/stringn.c utils/stdlib.c include/stdlib.h)

set_property(SOURCE crt0.S PROPERTY LANGUAGE C)
set_property(SOURCE sys/interrupt-entry.S PROPERTY LANGUAGE C)

set(K_ASM
        crt0.S
        sys/interrupt-entry.S)

add_executable(kernel_epita ${K_FILES} ${K_ASM} ${K_LIBS})