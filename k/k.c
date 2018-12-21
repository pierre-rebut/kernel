#include <stdio.h>
#include <cpu.h>
#include <multiboot.h>
#include <io/ata.h>
#include <sys/syscall.h>
#include <include/multiboot.h>
#include <io/fs/procfilesystem.h>

#include "io/serial.h"
#include "sys/gdt.h"
#include "sys/idt.h"
#include "io/pic.h"
#include "io/keyboard.h"
#include "io/pit.h"
#include "io/fs/kfilesystem.h"
#include "io/terminal.h"
#include "task.h"
#include "sys/allocator.h"
#include "sys/physical-memory.h"
#include "sys/paging.h"
#include "sheduler.h"
#include "sys/console.h"

#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)

static int k_init(const multiboot_info_t *info) {
    initSerial(38400);
    kSerialPrintf("Init Serial\n");

    kSerialPrintf("Init Terminal\n");
    initTerminal();

    LOG("Init memory\n");
    kprintf("Init Memory\n");
    initMemory();

    LOG("Init Physical Memory\n");
    kprintf("Init Physical Memory\n");
    u32 memSize = initPhysicalMemory(info);
    kprintf("Total memory size: %u\n", memSize);

    LOG("Init paging\n");
    kprintf("Init Paging\n");
    initPaging(memSize);

    LOG("Init allocator\n");
    kprintf("Init Allocator\n");
    if (initAllocator())
        return -1;

    LOG("Init interrupt\n");
    kprintf("Init Interrupt\n");
    initInterrupt();
    initPic();
    initPit();
    initKeyboard();
    initSyscall();

    LOG("Init console\n");
    kprintf("Init Console\n");
    initConsole();

    kprintf("Init KFileSystem\n");
    initKFileSystem();

    kprintf("Init ProcFileSystem\n");
    initProcFileSystem();

    kprintf("Mount kfs on A\n");
    struct Fs *kfs = fsGetFileSystemByName("kfs");
    struct FsVolume *kvolume = fsVolumeOpen('A', kfs, (void*) ((module_t*)(info->mods_addr))->mod_start);
    if (!kvolume)
        return -1;

    kprintf("Mount procfs on B\n");
    struct Fs *procfs = fsGetFileSystemByName("proc");
    struct FsVolume *pvolume = fsVolumeOpen('B', procfs, NULL);
    if (!pvolume)
        return -1;

    kprintf("Init Tasking\n");
    initTasking(fsVolumeRoot(kvolume));

    kprintf("Allow KEYBOARD & PIT interrupt\n");
    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);

    kprintf("Start listening interruption\n");
    sti();

    //kprintf("Init ATAPI\n");
   // ata_init();
    return 0;
}

void printfile(const char *pathname) {
    char *data = NULL;

    struct FsPath *file = fsResolvePath(pathname);
    if (!file)
        return;

    kSerialPrintf("File found\n");

    struct stat fileStat;
    if (fsStat(file, &fileStat) == -1)
        goto failure;

    kSerialPrintf("Stat found: %d\n", fileStat.file_sz);
    s32 fileSize = fileStat.file_sz;

    data = kmalloc(sizeof(char) * fileSize, 0, "datale");
    if (data == NULL)
        goto failure;

    kSerialPrintf("alloc ok\n");

    s32 readSize = fsReadFile(file, data, (u32) fileSize, 0);
    if (readSize != fileSize)
        goto failure;

    kSerialPrintf("read ok\n");

    writeSerial(data, (u32)readSize);

    failure:
    kSerialPrintf("end\n");
    fsPathDestroy(file);
    kfree(data);
}

void k_main(unsigned long magic, multiboot_info_t *info) {
    taskSwitching = 0;

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || info->mods_count != 1)
        goto error;

    if (k_init(info))
        goto error;

    kSerialPrintf("Read test file\n");
    printfile("/test");

    kSerialPrintf("\n### Trying init binary [%s] ###\n\n", (char *) info->cmdline);

    const char *av[] = {
            (char*)info->cmdline,
            NULL
    };
    const char *env[] = {
            "PATH=A:/",
            "HOME=/",
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
