//
// Created by rebut_p on 28/09/18.
//

#include "task.h"
#include "sys/idt.h"
#include "sys/gdt.h"
#include "io/kfilesystem.h"
#include "sys/allocator.h"
#include "sys/paging.h"
#include "elf.h"

#include <stdio.h>

struct task {
    u32 esp;
    u32 dataSegment;
    u32 brk;
};

static struct task myTask = {0};
static struct esp_context appStack = {0};

void launchTask() {
    asm volatile("int $50");
}

static void addTask(u32 entry, u32 esp) {
    appStack.ss = 0x23;
    appStack.useresp = esp + 0x1500000;
    appStack.eflags = 0x0202;

    appStack.cs = 0x1B;
    appStack.eip = entry;
    kSerialPrintf("eip: %d (%d), esp: %d\n", appStack.eip, *((u32 *) appStack.eip), appStack.useresp);

    appStack.gs = 0x23;
    appStack.fs = 0x23;
    appStack.es = 0x23;
    appStack.ds = 0x23;

    myTask.dataSegment = 0x23;
    myTask.esp = (u32) &appStack;
    myTask.brk = esp + 0x1500000 + 1;
}

int createTask(const char *cmdline) {
    s32 fileSize = length(cmdline);
    if (fileSize == 0) {
        kSerialPrintf("Can not get file info: %s\n", cmdline);
        return -1;
    }

    kSerialPrintf("task: openfile\n");
    int fd = open(cmdline, O_RDONLY);
    if (fd < 0) {
        kSerialPrintf("Can not open file: %s\n", cmdline);
        return -1;
    }

    kSerialPrintf("task: kmalloc\n");
    char *data = kmalloc(sizeof(char) * fileSize, 0);
    if (data == NULL) {
        close(fd);
        kSerialPrintf("Can not alloc memory\n");
        return -1;
    }

    kSerialPrintf("task: read\n");
    if (read(fd, data, fileSize) != (s32)fileSize) {
        kfree(data);
        close(fd);
        kSerialPrintf("Can not read bin data\n");
        return -1;
    }

    close(fd);
    kSerialPrintf("task: alloc pagedirec\n");
    struct PageDirectory *pageDirectory = pagingCreatePageDirectory();
    if (pageDirectory == NULL) {
        kfree(data);
        kSerialPrintf("Can not alloc new page directory\n");
        return -1;
    }

    kSerialPrintf("task: loadbin\n");
    u32 entryPrg = loadBinary(pageDirectory, data, fileSize);
    kfree(data);

    if (entryPrg == 0) {
        kSerialPrintf("Can not load binary: %s\n", cmdline);
        return -1;
    }
    kSerialPrintf("task: add task\n");
    addTask(entryPrg, 0);
    kSerialPrintf("task: end\n");
    return 0;
}

u32 task_switch(u32 previousEsp) {
    switchTSS(previousEsp, myTask.esp, myTask.dataSegment);
    return myTask.esp;
}

u32 sbrk(s32 inc) {
    if (inc == 0)
        return myTask.brk;

    u32 brk = myTask.brk;
    myTask.brk += inc;

    if (myTask.brk <= myTask.esp)
        myTask.brk = myTask.esp + 1;

    return brk;
}
