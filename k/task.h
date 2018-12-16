//
// Created by rebut_p on 28/09/18.
//

#include "include/multiboot.h"

#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#include <k/types.h>
#include <k/kstd.h>

#include "io/filesystem2.h"

struct Console;

enum TaskPrivilege {
    TaskPrivilegeKernel,
    TaskPrivilegeUser
};

enum TaskEventType {
    TaskEventNone,
    TaskEventKeyboard,
    TaskEventTimer,
    TaskEventWaitPid,
    TaskEventKernel
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
    struct Console *console;

    struct TaskEvent event;

    struct PageDirectory *pageDirectory;
    struct FileDescriptor *lstFiles[MAX_NB_FILE];

    struct fs_dirent *currentDir;
};

extern char taskSwitching;
extern struct Task *currentTask;
extern struct Task *freeTimeTask;

void taskSaveState(u32 esp);
u32 taskSwitch(struct Task *newTask);

struct Task *createTask(struct PageDirectory *pageDirectory, u32 entryPoint, enum TaskPrivilege privilege,
                        u32 ac, const char **av, const char **env, const char *dir, struct Console *console);
u32 createProcess(const char *cmdline, const char **av, const char **env);
void initTasking();

int taskKill(struct Task *);
int taskExit();
void taskWaitEvent(enum TaskEventType event, u32 arg);
u32 taskGetpid();
u32 taskKillByPid(u32 pid);
struct Task *getTaskByPid(u32 pid);

u32 taskSetHeapInc(s32 size);

int taskChangeDirectory(const char *directory);

#endif //KERNEL_EPITA_USERLAND_H