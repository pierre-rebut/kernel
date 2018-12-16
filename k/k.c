#include <stdio.h>
#include <cpu.h>
#include <multiboot.h>
#include <io/ata.h>
#include <sys/syscall.h>

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
#include "console.h"

static int k_init(const multiboot_info_t *info) {
    initSerial(38400);
    kSerialPrintf("Init Serial\n");

    kSerialPrintf("Init Terminal\n");
    initTerminal();

    kprintf("Init Memory\n");
    initMemory();

    kprintf("Init Physical Memory\n");
    u32 memSize = initPhysicalMemory(info);
    kprintf("Total memory size: %u\n", memSize);

    kprintf("Init Paging\n");
    initPaging(memSize);

    kprintf("Init Allocator\n");
    if (initAllocator())
        return -1;

    kprintf("Init Interrupt\n");
    initInterrupt();
    initPic();
    initPit();
    initKeyboard();
    initSyscall();

    kprintf("Init Console\n");
    initConsole();

    kprintf("Init Tasking\n");
    initTasking();

    kprintf("Allow KEYBOARD & PIT interrupt\n");
    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);

    kprintf("Init KFileSystem\n");
    initKFileSystem((module_t *) info->mods_addr);

    kprintf("Start listening interruption\n");
    sti();

    kprintf("Init ATAPI\n");
    ata_init();
    return 0;
}

void k_main(unsigned long magic, multiboot_info_t *info) {
    taskSwitching = 0;

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || info->mods_count != 1)
        goto error;

    if (k_init(info))
        goto error;

    kSerialPrintf("\n### Trying init binary [%s] ###\n\n", (char *) info->cmdline);

    const char *av[] = {
            (char*)info->cmdline,
            NULL
    };
    const char *env[] = {
            "PATH=/",
            "PWD=/",
            NULL
    };

    while (1) {
        clearTerminal();
        u32 pid = createProcess((char*)info->cmdline, av, env);
        taskWaitEvent(TaskEventWaitPid, pid);
        kprintf("Resetting terminal\n");
        taskWaitEvent(TaskEventTimer, 1000);
    }


    error:
    kSerialPrintf("An error occurred\n");
    cli();
    hlt();
}
