//
// Created by rebut_p on 28/09/18.
//

#include "task.h"
#include "sys/gdt.h"
#include "sys/allocator.h"
#include "sys/paging.h"
#include "elf.h"
#include "sheduler.h"
#include "sys/console.h"

#include <stdio.h>
#include <sys/physical-memory.h>
#include <io/fs/kfilesystem.h>
#include <cpu.h>
#include <string.h>
#include <io/terminal.h>
#include <io/serial.h>
#include <io/pit.h>
#include <io/keyboard.h>
#include <include/list.h>

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
#define LOG(x, ...)

static u32 nextPid = 0;

char taskSwitching = 0;
struct Task *currentTask = NULL;
struct Task *freeTimeTask = NULL;
struct Task kernelTask = {0};

void initTasking(struct FsPath *rootDirectory) {
    kernelTask.pid = 0;
    kernelTask.ss = 0x10;
    kernelTask.event.type = TaskEventNone;
    kernelTask.pageDirectory = kernelPageDirectory;
    kernelTask.privilege = TaskPrivilegeKernel;
    kernelTask.console = kernelConsole;
    kernelTask.cmdline = strdup("kernelTask");
    kernelConsole->task = &kernelTask;

    kernelTask.currentDir = rootDirectory;

    for (int i = 0; i < MAX_NB_FILE; i++)
        kernelTask.objectList[i] = NULL;

    struct TaskCreator taskInfo = {
            .pageDirectory = kernelPageDirectory,
            .entryPoint = (u32)&schedulerDoNothing,
            .privilege = TaskPrivilegeKernel,
            .ac = 0,
            .av = NULL,
            .env = NULL,
            .dir = kernelTask.currentDir,
            .console = kernelConsole,
            .cmdline = "kernelTask"
    };

    freeTimeTask = createTask(&taskInfo);
    currentTask = &kernelTask;
    schedulerAddTask(&kernelTask);

    taskSwitching = 1;
}

struct Task *createTask(struct TaskCreator *info) {
    struct Task *task = kmalloc(sizeof(struct Task), 0, "task");
    u32 *stack = kmalloc(KERNEL_STACK_SIZE, 0, "stack");
    if (!task || !stack)
        return NULL;

    task->pid = nextPid++;
    task->privilege = info->privilege;
    task->pageDirectory = info->pageDirectory;
    stack = (void *) ((u32) stack + KERNEL_STACK_SIZE);
    task->kernelStack = stack;
    task->event.type = TaskEventNone;
    task->heap.start = USER_HEAP_START;
    task->heap.pos = task->heap.nbPage = 0;
    task->console = info->console;

    task->cmdline = strdup(info->cmdline);

    task->currentDir = info->dir;
    info->dir->refcount++;

    for (int i = 0; i < MAX_NB_FILE; i++)
        task->objectList[i] = NULL;

    if (info->privilege == TaskPrivilegeUser)
        pagingAlloc(info->pageDirectory, (void *) (USER_STACK - 10 * PAGESIZE), 10 * PAGESIZE, MEM_USER | MEM_WRITE);

    u8 codeSegment = 0x8;
    u8 dataSegment = 0x10;

    if (info->privilege == TaskPrivilegeUser) {
        *(--stack) = 0x23;
        *(--stack) = USER_STACK;
        codeSegment = 0x18 | 0x3;
        dataSegment = 0x23;
    }

    *(--stack) = 0x0202;

    *(--stack) = codeSegment;       // cs
    *(--stack) = info->entryPoint;  // eip

    *(--stack) = 0;                 // error code
    *(--stack) = 0;                 // Interrupt nummer

    *(--stack) = info->ac;                // eax. Used to give argc to user programs.
    *(--stack) = (u32) info->av;    // ecx. Used to give argv to user programs.
    *(--stack) = (u32) info->env;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;

    *(--stack) = dataSegment;
    *(--stack) = dataSegment;
    *(--stack) = dataSegment;
    *(--stack) = dataSegment;

    task->esp = (u32) stack;
    task->ss = dataSegment;

    return task;
}

static const char **copyArrayIntoUserland(struct PageDirectory *pd, u32 position, u32 ac, const char **av) {
    char **nnArgv = 0;
    char *nArgv[ac];

    for (u32 i = 0; i < ac; i++) {
        nArgv[i] = strdup(av[i]);
    }

    cli();
    pagingSwitchPageDirectory(pd);
    nnArgv = (void *) position;
    void *addr = nnArgv + sizeof(char *) * ac;

    for (u32 i = 0; i < ac; i++) {
        size_t argsize = strlen(nArgv[i]) + 1;
        nnArgv[i] = addr;
        memcpy(nnArgv[i], nArgv[i], argsize);
        nnArgv[i][argsize] = '\0';
        addr += argsize + 1;
    }

    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();

    for (size_t index = 0; index < ac; index++)
        kfree(nArgv[index]);

    return (const char **)nnArgv;
}

u32 createProcess(const char *cmdline, const char **av, const char **env) {
    LOG("task: openfile\n");
    struct FsPath *file = fsResolvePath(cmdline);
    if (!file) {
        kSerialPrintf("[TASK] Can not open file: %s\n", cmdline);
        return 0;
    }

    LOG("task: get file stat\n");
    struct stat fileStat;
    if (fsStat(file, &fileStat) == -1) {
        kSerialPrintf("[TASK] Can not get file info: %s\n", cmdline);
        return 0;
    }

    s32 fileSize = fileStat.file_sz;

    LOG("task: kmalloc of size %d\n", fileSize);
    char *data = kmalloc(sizeof(char) * fileSize, 0, "data bin file");
    if (data == NULL) {
        fsPathDestroy(file);
        kSerialPrintf("[TASK] Can not alloc memory\n");
        return 0;
    }

    LOG("task: read\n");
    s32 readSize = fsReadFile(file, data, (u32) fileSize, 0);
    fsPathDestroy(file);

    if (readSize != (s32) fileSize) {
        kfree(data);
        kSerialPrintf("[TASK] Can not read bin data: %d\n", readSize);
        return 0;
    }

    LOG("task: alloc pagedirec\n");
    struct PageDirectory *pageDirectory = pagingCreatePageDirectory();
    if (pageDirectory == NULL) {
        kfree(data);
        kSerialPrintf("[TASK] Can not alloc new page directory\n");
        return 0;
    }

    LOG("task: loadbin\n");
    u32 entryPrg = loadBinary(pageDirectory, data, (u32) fileSize);
    kfree(data);

    if (entryPrg == 0) {
        kSerialPrintf("[TASK] Can not load binary: %s\n", cmdline);
        return 0;
    }

    u32 ac;
    for (ac = 0; av[ac] != NULL; ac++);
    u32 envc;
    for (envc = 0; env[envc] != NULL; envc++);

    pagingAlloc(pageDirectory, (void *) USER_STACK, (u32) USER_HEAP_START - (u32) USER_STACK, MEM_USER | MEM_WRITE);
    const char **newAv = copyArrayIntoUserland(pageDirectory, USER_ARG_BUFFER, ac, av);
    const char **newEnv = copyArrayIntoUserland(pageDirectory, USER_ENV_BUFFER, envc, env);

    LOG("task: create new console\n");
    struct Console *console = createConsole();
    if (!console)
        return 0;

    LOG("task: add task (entry: %X)\n", entryPrg);
    struct TaskCreator taskInfo = {
            .pageDirectory = pageDirectory,
            .entryPoint = entryPrg,
            .privilege = TaskPrivilegeUser,
            .ac = ac,
            .av = newAv,
            .env = newEnv,
            .dir = currentTask->currentDir,
            .console = console,
            .cmdline = av[0]
    };

    struct Task *task = createTask(&taskInfo);
    if (!task)
        return 0;

    console->task = task;

    task->objectList[0] = koCreate(KO_CONS, console, O_RDONLY);
    task->objectList[1] = koCreate(KO_CONS, console, O_WRONLY);
    task->objectList[2] = koCreate(KO_ERROR, console, O_WRONLY);

    schedulerAddTask(task);

    setActiveConsole(console);

    LOG("task: end\n");
    return task->pid;
}

int taskKill(struct Task *task) {
    LOG("Killing task: %u\n", task->pid);

    if (task->pid == 0) {
        kSerialPrintf("can not kill kernel task !!\n");
        return -1;
    }

    char tmpTaskSwitching = taskSwitching;
    taskSwitching = 0;

    for (int fd = 0; fd < MAX_NB_FILE; fd++) {
        struct Kobject *obj = task->objectList[fd];
        if (obj)
            koDestroy(obj);
    }

    kfree((void*)task->cmdline);
    kfree(task->kernelStack - KERNEL_STACK_SIZE);

    LOG("[TASK] destroy path curDir\n");
    fsPathDestroy(task->currentDir);

    pagingDestroyPageDirectory(task->pageDirectory);

    destroyConsole(task->console);
    schedulerRemoveTask(task);

    kfree(task);
    taskSwitching = tmpTaskSwitching;

    if (currentTask == task) {
        hlt();
    }

    return 0;
}

u32 taskGetpid() {
    return currentTask->pid;
}

u32 taskKillByPid(u32 pid) {
    struct Task *task = getTaskByPid(pid);
    if (task == NULL)
        return 0;

    taskKill(task);
    return pid;
}

int taskExit() {
    return taskKill(currentTask);
}

void taskWaitEvent(enum TaskEventType event, u32 arg) {
    currentTask->event.type = event;
    currentTask->event.timer = gettick();
    currentTask->event.arg = arg;
    schedulerForceSwitchTask();
}

void taskSaveState(u32 esp) {
    currentTask->esp = esp;
}

u32 taskSwitch(struct Task *newTask) {
    taskSwitching = 0;
    currentTask = newTask;

    switchTSS((u32) newTask->kernelStack, newTask->esp, newTask->ss);
    pagingSwitchPageDirectory(newTask->pageDirectory);

    taskSwitching = 1;
    // kSerialPrintf("Task switch to pid %u\n", newTask->pid);
    return newTask->esp;
}

u32 taskSetHeapInc(s32 inc) {
    if (currentTask->pid == 0)
        return 0;

    struct Heap *heap = &currentTask->heap;
    if (inc == 0)
        return heap->start + heap->pos;

    u32 brk = heap->pos;

    if (inc > 0) {
        if ((u32) inc > (heap->nbPage * PAGESIZE) - heap->pos) {
            u32 incPage = alignUp((u32) inc, PAGESIZE);
            pagingAlloc(currentTask->pageDirectory, (void *) (heap->start + heap->nbPage * PAGESIZE), incPage,
                        MEM_WRITE | MEM_USER);

            heap->nbPage += incPage / PAGESIZE;
        }
    } else {
        inc = -inc;
        if ((u32) inc < (heap->nbPage * PAGESIZE) - heap->pos) {
            u32 decPage = alignDown((u32) inc, PAGESIZE);
            u32 decPos = heap->start + heap->nbPage * PAGESIZE - decPage;
            pagingFree(currentTask->pageDirectory, (void *) decPos, decPage);
            heap->nbPage -= decPage / PAGESIZE;
        }

    }

    heap->pos += inc;
    return heap->start + brk;
}

struct Task *getTaskByPid(u32 pid) {
    if (taskLists.begin == NULL)
        return NULL;

    struct ListElem *elem = taskLists.begin;
    while (elem != NULL) {
        struct Task *task = (struct Task *)elem->data;
        if (task->pid == pid)
            return task;
        elem = elem->next;
    }
    return NULL;
}

int taskChangeDirectory(const char *directory) {
    struct FsPath *newPath = fsResolvePath(directory);
    if (!newPath)
        return -1;

    fsPathDestroy(currentTask->currentDir);
    currentTask->currentDir = newPath;
    return 0;
}

int taskGetAvailableFd(struct Task *task) {
    int id;
    for (id = 0; id < MAX_NB_FILE; id++)
        if (task->objectList[id] == NULL)
            break;

    if (id >= MAX_NB_FILE)
        return -1;

    return id;
}
