include_directories(.)
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/libs/libc/include/kernel)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/k.lds")

set(K_FILES
        system/allocator.c system/allocator.h
        elf.h elf.c
        include/compiler.h include/cpu.h io/io.h include/multiboot.h
        system/gdt.c system/gdt.h
        system/idt.c system/idt.h
        system/interrupt-handler.c
        k.c
        io/keyboard.c io/keyboard.h
        io/fs/kfilesystem.c io/fs/kfilesystem.h
        io/libvga.c io/libvga.h
        system/paging.c system/paging.h
        system/physical-memory.c system/physical-memory.h
        io/pic.c io/pic.h
        io/pit.c io/pit.h
        io/serial.c io/serial.h
        system/syscalls.c system/syscall.h
        task.c task.h
        io/terminal.c io/terminal.h
        io/fs/filesystem.c io/fs/filesystem.h
        sheduler.c sheduler.h
        utils/list.c include/list.h
        io/device/ata.c io/device/ata.h
        system/console.c system/console.h
        system/kobject.c system/kobject.h
        io/fs/procfilesystem.c io/fs/procfilesystem.h
        io/cmos.c io/cmos.h
        system/time.c system/time.h system/mutex.c system/mutex.h io/device/device.c io/device/device.h
        io/fs/devfilesystem.c io/fs/devfilesystem.h io/fs/ext2filesystem.c
        io/fs/ext2filesystem.h io/device/fscache.c io/device/fscache.h io/pipe.c
        io/pipe.h tty.c tty.h io/device/proc.c io/device/proc.h)

set(K_UTILS
        utils/kprintf.c
        utils/list.c
        utils/string.c
        include/ascii.h)

set_property(SOURCE crt0.S PROPERTY LANGUAGE C)
set_property(SOURCE system/interrupt-entry.S PROPERTY LANGUAGE C)

set(K_ASM
        crt0.S
        system/interrupt-entry.S)

add_executable(kernel_epita ${K_FILES} ${K_ASM} ${K_UTILS})
target_link_libraries(kernel_epita stdk)