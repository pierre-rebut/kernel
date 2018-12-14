//
// Created by rebut_p on 28/09/18.
//

#include "include/multiboot.h"

#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#include <k/types.h>
#include <k/kstd.h>

#include "io/filesystem.h"

enum TaskEvent {
    TaskEventNone,
    TaskEventKeyboard,
    TaskEventTimer,
    TaskEventWaitPid
};

struct Heap {
    u32 start;
    u32 nbPage;
    u32 pos;
};

struct Task {
    u32 pid;

    u32 esp;
    u32 ss;
    void *kernelStack;
    struct Heap heap;

    enum TaskEvent event;
    u32 eventArg;

    struct PageDirectory *pageDirectory;
    struct FileDescriptor *lstFiles[255];

    struct Task *next;
    struct Task *prev;

} __attribute__((packed));

extern char taskSwitching;
extern struct Task *currentTask;
extern struct Task *lstTasks;
extern struct Task kernelTask;

void taskSaveState(u32 esp);
u32 taskSwitch(struct Task *newTask);

int createProcess(const char *cmdline);
void initTasking();

int taskKill(struct Task *);
int taskExit();
void taskAddEvent(enum TaskEvent event, u32 arg);
u32 taskGetpid();
int taskKillByPid(u32 pid);

u32 taskSetHeapInc(s32 size);

#endif //KERNEL_EPITA_USERLAND_H
