#include <stdio.h>
#include <cpu.h>
#include <multiboot.h>

#include "io/serial.h"
#include "sys/gdt.h"
#include "sys/idt.h"
#include "io/pic.h"
#include "io/keyboard.h"
#include "io/pit.h"
#include "io/kfilesystem.h"
#include "io/terminal.h"
#include "task.h"
#include "sys/allocator.h"
#include "sys/physical-memory.h"
#include "sys/paging.h"
#include "sheduler.h"

static int k_init(const multiboot_info_t *info) {
    initSerial(38400);
    kSerialPrintf("Init Serial\n");

    kSerialPrintf("Init Memory\n");
    initMemory();

    kSerialPrintf("Init Physical Memory\n");
    u32 memSize = initPhysicalMemory(info);
    kSerialPrintf("Total memory size: %u\n", memSize);

    kSerialPrintf("Init Paging\n");
    initPaging(memSize);

    kSerialPrintf("Init Allocator\n");
    if (initAllocator())
        return -1;

    kSerialPrintf("Init Terminal\n");
    initTerminal();

    kSerialPrintf("Init Interrupt\n");
    initInterrupt();

    kSerialPrintf("Init Pic\n");
    initPic();

    kSerialPrintf("Init Pit\n");
    initPit();

    kSerialPrintf("Init Tasking\n");
    initTasking();

    kSerialPrintf("Allow KEYBOARD & PIT interrupt\n");
    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);

    kSerialPrintf("Init KFileSystem\n");
    initKFileSystem((module_t *) info->mods_addr);

    kSerialPrintf("Start listening interruption\n");
    sti();
    return 0;
}

static void k_test() {
    kfsListFiles();
    int fd = open("test", O_RDONLY);
    kSerialPrintf("Open file: %d\n", fd);

    if (fd >= 0) {
        char buf[100];
        u32 i = 0, fileLength = kfsLengthOfFile("test");
        kSerialPrintf("len of file = %d\n", fileLength);

        while (i < fileLength) {
            int tmp = read(fd, buf, 99);
            buf[tmp] = 0;

            i += tmp;
            kSerialPrintf("%s", buf);
        }
        kSerialPrintf("\nClosing file: %d\n", close(fd));
    }
}

void my_putnbr(unsigned long n, u32 pos) {
    if (n > 9) {
        my_putnbr(n / 10, pos++);
        my_putnbr(n % 10, pos++);
    } else
        writeTerminalAt(n + '0', CONS_GREEN, pos, 24);
}

void k_main(unsigned long magic, multiboot_info_t *info) {
    taskSwitching = 0;

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || info->mods_count != 1)
        goto error;

    if (k_init(info))
        goto error;

    kSerialPrintf("\n### Starting kernel test ###\n\n");
    k_test();
    kSerialPrintf("\n### Kernel test ok ###\n### Trying init binary [%s] ###\n\n", (char *) info->cmdline);

    const char *av[] = {
            (char*)info->cmdline,
            NULL
    };
    const char *env[] = {
            "PATH=/",
            "PWD=/",
            NULL
    };

    createProcess((char*)info->cmdline, av, env);

    kSerialPrintf("Kernel loop start\n");
    taskSwitching = 1;

    schedulerDoNothing();

    error:
    kSerialPrintf("An error occurred\n");
    cli();
    hlt();
}
