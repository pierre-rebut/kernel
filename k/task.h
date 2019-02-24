//
// Created by rebut_p on 28/09/18.
//
#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#include <ctype.h>
#include <list.h>

#include "io/fs/filesystem.h"
#include "system/kobject.h"

#define TASK_MAX_PID 1024

struct Console;

enum TaskPrivilege
{
    TaskPrivilegeKernel,
    TaskPrivilegeUser
};

enum TaskEventType
{
    TaskEventNone,
    TaskEventKeyboard,
    TaskEventTimer,
    TaskEventWaitPid,
    TaskEventMutex,
    TaskEventPipe
};

struct Heap
{
    u32 start;
    u32 nbPage;
    u32 pos;
};

struct TaskEvent
{
    enum TaskEventType type;
    unsigned long timer;
    u32 arg;
};

enum TaskType
{
    T_PROCESS,
    T_THREAD
};

struct TaskCreator
{
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

struct Task
{
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
    struct Kobject *objectList[MAX_NB_KOBJECT];

    struct FsPath *currentDir;
    struct FsPath *rootDir;
    struct Console *console;

    struct List childs;
};

extern char taskSwitching;
extern struct Task *currentTask;
extern struct Task *freeTimeTask;
extern struct Task kernelTask;

void taskSaveState(u32 esp);

u32 taskSwitch(struct Task *newTask);

struct Task *createTask(struct TaskCreator *info);

pid_t createProcess(const struct ExceveInfo *info);

pid_t createThread(u32 entryPrg);

void initTasking();

int taskKill(struct Task *task);

int taskExit();

void taskWaitEvent(enum TaskEventType event, u32 arg);

void taskResetEvent(struct Task *task);

pid_t taskGetpid();

int taskKillByPid(pid_t pid);

struct Task *getTaskByPid(pid_t pid);

u32 taskHeapInc(s32 size);

int taskHeapSet(u32 addr);

int taskGetAvailableFd(struct Task *task);

struct Kobject *taskGetKObjectByFd(int fd);

int taskSetKObjectByFd(int fd, struct Kobject *obj);

#endif //KERNEL_EPITA_USERLAND_H
