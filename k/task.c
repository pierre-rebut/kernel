//
// Created by rebut_p on 28/09/18.
//

#include "task.h"
#include "idt.h"
#include "gdt.h"

#include <stdio.h>

struct task {
    u32 esp;
    u32 dataSegment;
};

static struct task myTask = {0};
static struct esp_context appStack = {0};

void createTask(u32 entry, u32 esp) {
    appStack.ss = 0x23;
    appStack.useresp = esp + 0x1500000;
    appStack.eflags = 0x0202;

    appStack.cs = 0x1B;
    appStack.eip = entry;
    printf("eip: %d (%d), esp: %d\n", appStack.eip, *((u32*)appStack.eip), appStack.useresp);

    appStack.gs = 0x23;
    appStack.fs = 0x23;
    appStack.es = 0x23;
    appStack.ds = 0x23;

    myTask.dataSegment = 0x23;
    myTask.esp = (u32)&appStack;
}

u32 task_switch(u32 previousEsp) {
    switchTSS(previousEsp, myTask.esp, myTask.dataSegment);
    return myTask.esp;
}
