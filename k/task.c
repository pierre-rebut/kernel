//
// Created by rebut_p on 28/09/18.
//

#include "task.h"
#include "sys/gdt.h"
#include "sys/allocator.h"
#include "sys/paging.h"
#include "elf.h"
#include "sheduler.h"

#include <stdio.h>
#include <sys/physical-memory.h>
#include <io/kfilesystem.h>
#include <io/terminal.h>
#include <io/serial.h>
#include <io/pit.h>
#include <include/cpu.h>
#include <string.h>
#include <sys/allocator.h>

static u32 nextPid = 1;

char taskSwitching = 0;
struct Task *lstTasks = NULL;
struct Task *currentTask = NULL;
struct Task kernelTask = {
        .pid = 0,
        .privilege = TaskPrivilegeKernel,
        .esp = 0,
        .ss = 0x10,
        .event.type = TaskEventNone,
        .heap = {0}
};

void initTasking() {
    kernelTask.pageDirectory = kernelPageDirectory;
    kernelTask.lstFiles[0] = createFileDescriptor(NULL, NULL, NULL, NULL);
    kernelTask.lstFiles[1] = createFileDescriptor(NULL, &writeTerminalFromFD, NULL, NULL);
    kernelTask.lstFiles[2] = createFileDescriptor(NULL, &writeSerialFromFD, NULL, NULL);

    for (int i = 3; i < MAX_NB_FILE; i++)
        kernelTask.lstFiles[i] = NULL;

    currentTask = &kernelTask;
}

struct Task *createTask(struct PageDirectory *pageDirectory, u32 entryPoint, enum TaskPrivilege privilege,
                        u32 ac, const char **av, const char **env) {
    struct Task *task = kmalloc(sizeof(struct Task), 0, "task");
    u32 *stack = kmalloc(KERNEL_STACK_SIZE, 0, "stack");
    if (!task || !stack)
        return NULL;

    task->pid = nextPid++;
    task->privilege = privilege;
    task->pageDirectory = pageDirectory;
    stack = (void *) ((u32) stack + KERNEL_STACK_SIZE);
    task->kernelStack = stack;
    task->event.type = TaskEventNone;
    task->heap.start = USER_HEAP_START;
    task->heap.pos = task->heap.nbPage = 0;

    task->lstFiles[0] = createFileDescriptor(NULL, NULL, NULL, NULL);
    task->lstFiles[1] = createFileDescriptor(NULL, &writeTerminalFromFD, NULL, NULL);
    task->lstFiles[2] = createFileDescriptor(NULL, &writeSerialFromFD, NULL, NULL);

    for (int i = 3; i < MAX_NB_FILE; i++)
        task->lstFiles[i] = NULL;

    if (privilege == TaskPrivilegeUser)
        pagingAlloc(pageDirectory, (void *) (USER_STACK - 10 * PAGESIZE), 10 * PAGESIZE, MEM_USER | MEM_WRITE);

    u8 codeSegment = 0x8;
    u8 dataSegment = 0x10;

    if (privilege == TaskPrivilegeUser) {
        *(--stack) = 0x23;
        *(--stack) = USER_STACK;
        codeSegment = 0x18 | 0x3;
        dataSegment = 0x23;
    }

    *(--stack) = 0x0202;

    *(--stack) = codeSegment; // cs
    *(--stack) = entryPoint; // eip

    *(--stack) = 0;               // error code
    *(--stack) = 0;               // Interrupt nummer

    // General purpose registers w/o esp
    *(--stack) = ac;            // eax. Used to give argc to user programs.
    *(--stack) = (u32) av; // ecx. Used to give argv to user programs.
    *(--stack) = (u32) env;
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

static void addTask(struct Task *task) {
    if (lstTasks == NULL) {
        task->prev = NULL;
        lstTasks = task;
    } else {
        struct Task *iter = lstTasks;
        while (iter->next != NULL)
            iter = iter->next;
        iter->next = task;
        task->prev = iter;
    }
    task->next = NULL;
}

static const char **copyArgEnvToUserland(struct PageDirectory *pd, u32 position, u32 ac, const char **av) {
    char **nnArgv = 0;
    char *nArgv[ac];

    for (u32 i = 0; i < ac; i++)
        nArgv[i] = strdup(av[i]);

    cli();
    pagingSwitchPageDirectory(pd);
    nnArgv = (void *) position;
    void *addr = nnArgv + sizeof(char *) * ac;

    for (u32 i = 0; i < ac; i++) {
        size_t argsize = strlen(nArgv[i]) + 1;
        nnArgv[i] = addr;
        memcpy(nnArgv[i], nArgv[i], argsize);
        addr += argsize;
    }

    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();

    for (size_t index = 0; index < ac; index++)
        kfree(nArgv[index]);

    return (const char **)nnArgv;
}

u32 createProcess(const char *cmdline, const char **av, const char **env) {
    s32 fileSize = kfsLengthOfFile(cmdline);
    if (fileSize == 0) {
        kSerialPrintf("Can not get file info: %s\n", cmdline);
        return 0;
    }

    kSerialPrintf("task: openfile\n");
    int fd = open(cmdline, O_RDONLY);
    if (fd < 0) {
        kSerialPrintf("Can not open file: %s\n", cmdline);
        return 0;
    }

    kSerialPrintf("task: kmalloc\n");
    char *data = kmalloc(sizeof(char) * fileSize, 0, "data bin file");
    if (data == NULL) {
        close(fd);
        kSerialPrintf("Can not alloc memory\n");
        return 0;
    }

    kSerialPrintf("task: read\n");
    if (read(fd, data, (u32) fileSize) != (s32) fileSize) {
        kfree(data);
        close(fd);
        kSerialPrintf("Can not read bin data\n");
        return 0;
    }

    close(fd);
    kSerialPrintf("task: alloc pagedirec\n");
    struct PageDirectory *pageDirectory = pagingCreatePageDirectory();
    if (pageDirectory == NULL) {
        kfree(data);
        kSerialPrintf("Can not alloc new page directory\n");
        return 0;
    }

    kSerialPrintf("task: loadbin\n");
    u32 entryPrg = loadBinary(pageDirectory, data, (u32) fileSize);
    kfree(data);

    if (entryPrg == 0) {
        kSerialPrintf("Can not load binary: %s\n", cmdline);
        return 0;
    }

    u32 ac;
    for (ac = 0; av[ac] != NULL; ac++);
    u32 envc;
    for (envc = 0; env[envc] != NULL; envc++);

    pagingAlloc(pageDirectory, (void *) USER_STACK, (u32) USER_HEAP_START - (u32) USER_STACK, MEM_USER | MEM_WRITE);
    const char **newAv = copyArgEnvToUserland(pageDirectory, USER_ARG_BUFFER, ac, av);
    const char **newEnv = copyArgEnvToUserland(pageDirectory, USER_ENV_BUFFER, envc, env);

    kSerialPrintf("task: add task\n");
    struct Task *task = createTask(pageDirectory, entryPrg, TaskPrivilegeUser, ac, newAv, newEnv);
    if (!task)
        return 0;

    addTask(task);

    kSerialPrintf("task: end\n");
    return task->pid;
}

int taskKill(struct Task *task) {
    kSerialPrintf("Killing task: %u\n", task->pid);

    if (task == &kernelTask) {
        kSerialPrintf("can not kill kernel task !!\n");
        return -1;
    }

    char tmpTaskSwitching = taskSwitching;
    taskSwitching = 0;

    for (int fd = 0; fd < MAX_NB_FILE; fd++) {
        struct FileDescriptor *file = task->lstFiles[fd];
        if (file) {
            if (file->closeFct) {
                file->closeFct(file->entryData);
            }
            kfree(file);
        }
    }
    kfree(task->kernelStack - KERNEL_STACK_SIZE);

    pagingDestroyPageDirectory(task->pageDirectory);

    if (task->next)
        task->next->prev = task->prev;

    if (task->prev)
        task->prev->next = task->next;
    else
        lstTasks = task->next;

    kfree(task);
    taskSwitching = tmpTaskSwitching;

    kSerialPrintf("end kill process\n");

    if (currentTask == task) {
        currentTask = &kernelTask;
        schedulerForceSwitchTask();
    }

    return 0;
}

u32 taskGetpid() {
    return currentTask->pid;
}

u32 taskKillByPid(u32 pid) {
    for (struct Task *task = lstTasks; task != NULL; task = task->next) {
        if (task->pid == pid) {
            taskKill(task);
            return pid;
        }
    }
    return 0;
}

int taskExit() {
    return taskKill(currentTask);
}

void taskAddEvent(enum TaskEventType event, u32 arg) {
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
    kSerialPrintf("Task switch to pid %d\n", newTask->pid);
    return newTask->esp;
}

u32 taskSetHeapInc(s32 inc) {
    if (currentTask == &kernelTask)
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
    if (lstTasks == NULL)
        return NULL;

    struct Task *task = lstTasks;
    while (task != NULL) {
        if (task->pid == pid)
            return task;
        task = task->next;
    }
    return NULL;
}
