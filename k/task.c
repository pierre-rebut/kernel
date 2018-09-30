//
// Created by rebut_p on 28/09/18.
//

#include "task.h"
#include "sys/idt.h"
#include "sys/gdt.h"
#include "sys/paging2.h"
#include "binary.h"
#include "kfilesystem.h"
#include "sys/kalloc.h"

#include <stdio.h>

struct task {
    u32 esp;
    u32 dataSegment;
    u32 brk;
    struct PageDirectory *pd;
};

static struct task myTask = {0};
static struct esp_context appStack = {0};

void launchTask() {
    asm volatile("int $50");
}

static char *getFileData(const char *file, u32 size) {
    printf("malloc 1\n");
    char *data = kmalloc(size, 0);
    printf("malloc 2\n");
    if (data == NULL)
        return NULL;

    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        kfree(data);
        return NULL;
    }

    ssize_t tmp = read(fd, data, size);
    close(fd);
    if (tmp != (ssize_t) size) {
        kfree(data);
        return NULL;
    }
    return data;
}

void execute(const char *cmdline) {
    u32 size = length(cmdline);
    if (size == 0) {
        printf("bin size 0\n");
        return;
    }

    printf("alloc data tmp\n");
    char *data = getFileData(cmdline, size);
    if (data == NULL) {
        printf("null data bin\n");
        return;
    }

    printf("creating page directory\n");
    struct PageDirectory *pd = createPageDirrectory();

    printf("loading binary\n");
    u32 entry = loadBinary(pd, data, size);
    if (entry == 0) {
        destroyPageDirectory(pd);
        printf("can 't execute bin\n");
        return;
    }

    printf("LOADING END - INIT STACK\n");

    paging_alloc(pd, (void *) (0x1500000 - 10 * PAGESIZE), 10 * PAGESIZE, MEM_USER | MEM_WRITE);

    myTask.pd = pd;
    createTask(entry, 0);
}

void createTask(u32 entry, u32 esp) {
    appStack.ss = 0x23;
    appStack.useresp = 0x1500000;
    appStack.eflags = 0x0202;

    appStack.cs = 0x1B;
    appStack.eip = entry;
    printf("eip: %d, esp: %d\n", appStack.eip, appStack.useresp);

    appStack.gs = 0x23;
    appStack.fs = 0x23;
    appStack.es = 0x23;
    appStack.ds = 0x23;

    myTask.dataSegment = 0x23;
    myTask.esp = (u32) &appStack;
    myTask.brk = 0x1500000 + 1;
}

u32 task_switch(u32 previousEsp) {
    switchTSS(previousEsp, myTask.esp, myTask.dataSegment);
    switchPaging(myTask.pd);
    return myTask.esp;
}

u32 sbrk(ssize_t inc) {
    if (inc == 0)
        return myTask.brk;

    printf("brk: %X\n", myTask.brk);

    return 0;
}
