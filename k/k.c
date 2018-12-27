#include <stdio.h>
#include <cpu.h>
#include <multiboot.h>
#include <io/device/ata.h>
#include <sys/syscall.h>
#include <include/multiboot.h>
#include <io/fs/procfilesystem.h>
#include <io/fs/devfilesystem.h>
#include <io/device/device.h>
#include <io/fs/ext2filesystem.h>

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
//#define LOG(x, ...)

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

    kprintf("Init DevFileSystem\n");
    initDevFileSystem();

    kprintf("Init Ext2FileSystem\n");
    initExt2FileSystem();

    kprintf("Init Tasking\n");
    initTasking();

    kprintf("Allow KEYBOARD & PIT interrupt\n");
    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);

    kprintf("Start listening interruption\n");
    sti();

    kprintf("Init ATA driver\n");
    ata_init();

    struct DeviceDriver *driverAta = deviceGetDeviceDriverByName("ata");
    struct FsVolume *kvolume = NULL;

    kprintf("Mount available ATA device\n");
    char mountId = 'A';
    struct Fs *extfs = fsGetFileSystemByName("ext2fs");
    for (u32 i = 0; i < 4; i++) {
        u32 nblocks = 0;
        int blocksize = 0;
        char longname[256];

        if (driverAta->probe(i, &nblocks, &blocksize, longname) == 1) {
            kSerialPrintf("Mounting unit %d on %c: %s\n", i, mountId, longname);
            kprintf("Mounting unit %d on %c: %s\n", i, mountId, longname);
            kvolume = fsVolumeOpen(mountId, extfs, i);
            if (kvolume == NULL)
                kSerialPrintf("Mounting failed\n");
            else
                mountId++;
        }
    }

    if (kvolume == NULL)
        return -1;

    kernelTask.currentDir = freeTimeTask->currentDir = fsVolumeRoot(kvolume);
    freeTimeTask->currentDir->refcount++;

    kprintf("Mount procfs on %c\n", mountId);
    struct Fs *procfs = fsGetFileSystemByName("procfs");
    struct FsVolume *pvolume = fsVolumeOpen(mountId++, procfs, 0);
    if (!pvolume)
        return -1;

    kprintf("Mount devfs on %c\n", mountId);
    struct Fs *devfs = fsGetFileSystemByName("devfs");
    struct FsVolume *dvolume = fsVolumeOpen(mountId, devfs, 0);
    if (!dvolume)
        return -1;

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

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        goto error;

    if (k_init(info))
        goto error;

    kSerialPrintf("Read test file\n");
    printfile("/test");

    taskWaitEvent(TaskEventTimer, 1000);
    kSerialPrintf("\n### Trying init binary [%s] ###\n\n", (char *) info->cmdline);

    const char *av[] = {
            (char*)info->cmdline,
            NULL
    };
    const char *env[] = {
            "PATH=A:/bin",
            "HOME=A:/home",
            "PWD=A:",
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
