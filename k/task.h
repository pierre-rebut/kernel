//
// Created by rebut_p on 28/09/18.
//

#include "include/multiboot.h"

#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#include <k/types.h>
#include <k/kstd.h>

#include "io/filesystem.h"

enum TaskPrivilege {
    TaskPrivilegeKernel,
    TaskPrivilegeUser
};

enum TaskEventType {
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

struct TaskEvent {
    enum TaskEventType type;
    unsigned long timer;
    u32 arg;
};

struct Task {
    u32 pid;
    enum TaskPrivilege privilege;

    u32 esp;
    u32 ss;
    void *kernelStack;
    struct Heap heap;

    struct TaskEvent event;

    struct PageDirectory *pageDirectory;
    struct FileDescriptor *lstFiles[MAX_NB_FILE];

    struct Task *next;
    struct Task *prev;

};

extern char taskSwitching;
extern struct Task *currentTask;
extern struct Task *lstTasks;
extern struct Task kernelTask;

void taskSaveState(u32 esp);
u32 taskSwitch(struct Task *newTask);

struct Task *createTask(struct PageDirectory *pageDirectory, u32 entryPoint, enum TaskPrivilege privilege,
                        u32 ac, const char **av, const char **env);
u32 createProcess(const char *cmdline, const char **av, const char **env);
void initTasking();

int taskKill(struct Task *);
int taskExit();
void taskAddEvent(enum TaskEventType event, u32 arg);
u32 taskGetpid();
u32 taskKillByPid(u32 pid);
struct Task *getTaskByPid(u32 pid);

u32 taskSetHeapInc(s32 size);

#endif //KERNEL_EPITA_USERLAND_H
