//
// Created by rebut_p on 28/09/18.
//

#include "task.h"
#include "sys/idt.h"
#include "sys/gdt.h"
#include "sys/paging.h"
#include "binary.h"

#include <stdio.h>

struct task {
    u32 esp;
    u32 dataSegment;
    u32 brk;
    pageDirectory_t *pd;
};

static struct task myTask = {0};
static struct esp_context appStack = {0};

void launchTask() {
    asm volatile("int $50");
}

void execute(const char *cmdline) {
    /*pageDirectory_t *pd = paging_createPageDirectory();

    u32 entry = loadBinary(pd, cmdline);
    if (entry == 0) {
        paging_destroyPageDirectory(pd);
        printf("can 't execute bin\n");
        return;
    }

    paging_alloc(pd, (void *) (0x1500000 - 10 * PAGESIZE), 10 * PAGESIZE, MEM_USER | MEM_WRITE);
    myTask.pd = pd;
    createTask(entry, 0);*/
}

void createTask(u32 entry, u32 esp) {
    appStack.ss = 0x23;
    appStack.useresp = 0x1500000;
    appStack.eflags = 0x0202;

    appStack.cs = 0x1B;
    appStack.eip = entry;
    printf("eip: %d (%d), esp: %d\n", appStack.eip, *((u32 *) appStack.eip), appStack.useresp);

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
    //paging_switch(myTask.pd);
    return myTask.esp;
}

u32 sbrk(ssize_t inc) {
    if (inc == 0)
        return myTask.brk;

    u32 brk = myTask.brk;
    myTask.brk += inc;

    if (myTask.brk <= myTask.esp)
        myTask.brk = myTask.esp + 1;

    return brk;
}
