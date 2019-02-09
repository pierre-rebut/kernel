//
// Created by rebut_p on 28/09/18.
//
#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#include <k/types.h>
#include <k/kstd.h>
#include <include/list.h>

#include "io/fs/filesystem.h"
#include "sys/kobject.h"

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
    TaskEventMutex,
    TaskEventPipe
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

enum TaskType {
    T_PROCESS,
    T_THREAD
};

struct TaskCreator {
    enum TaskType type;
    struct PageDirectory *pageDirectory;
    u32 entryPoint;
    enum TaskPrivilege privilege;
    const char *cmdline;

    u32 ac;
    const char **av;
    const char **env;
    void *parent;
};

struct Task {
    enum TaskType type;
    u32 pid;

    const char *cmdline;
    enum TaskPrivilege privilege;
    struct Task *parent;
    struct List threads;

    u32 esp;
    u32 ss;
    void *kernelStack;
    struct Heap heap;

    struct TaskEvent event;

    struct PageDirectory *pageDirectory;
    struct Kobject *objectList[MAX_NB_FILE];

    struct FsPath *currentDir;
    struct Console *console;
};

extern char taskSwitching;
extern struct Task *currentTask;
extern struct Task *freeTimeTask;
extern struct Task kernelTask;

void taskSaveState(u32 esp);
u32 taskSwitch(struct Task *newTask);

struct Task *createTask(struct TaskCreator *info);
u32 createProcess(const char *cmdline, const char **av, const char **env);
void initTasking();

int taskKill(struct Task *);
int taskExit();
void taskWaitEvent(enum TaskEventType event, u32 arg);
void taskResetEvent(struct Task *task);
u32 taskGetpid();
u32 taskKillByPid(u32 pid);
struct Task *getTaskByPid(u32 pid);

u32 taskSetHeapInc(s32 size);

int taskChangeDirectory(const char *directory);

int taskGetAvailableFd(struct Task *task);
struct Kobject *taskGetKObjectByFd(int fd);
int taskSetKObjectByFd(int fd, struct Kobject *obj);

#endif //KERNEL_EPITA_USERLAND_H
