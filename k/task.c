//
// Created by rebut_p on 28/09/18.
//

#include <cpu.h>
#include <string.h>
#include <errno-base.h>
#include <kstdio.h>

#include <io/pit.h>
#include <system/physical-memory.h>
#include <system/gdt.h>
#include <system/allocator.h>
#include <system/paging.h>
#include <system/console.h>

#include "task.h"
#include "elf.h"
#include "sheduler.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

char taskSwitching = 0;
struct Task *currentTask = NULL;
struct Task *freeTimeTask = NULL;
struct Task kernelTask = {0};

static struct Task *tasksTable[TASK_MAX_PID] = {0};

void initTasking()
{
    kernelTask.type = T_PROCESS;
    kernelTask.pid = 0;
    tasksTable[0] = &kernelTask;
    kernelTask.ss = 0x10;
    kernelTask.event.type = TaskEventNone;
    kernelTask.pageDirectory = kernelPageDirectory;
    kernelTask.privilege = TaskPrivilegeKernel;
    kernelTask.cmdline = strdup("kernelTask");
    kernelTask.parent = NULL;

    for (int i = 0; i < MAX_NB_KOBJECT; i++)
        kernelTask.objectList[i] = NULL;

    struct TaskCreator taskInfo = {
            .type = T_PROCESS,
            .pageDirectory = kernelPageDirectory,
            .entryPoint = (u32) &schedulerDoNothing,
            .privilege = TaskPrivilegeKernel,
            .ac = 0,
            .av = NULL,
            .env = NULL,
            .cmdline = kernelTask.cmdline,
            .parent = &kernelTask
    };

    freeTimeTask = createTask(&taskInfo);

    currentTask = &kernelTask;
    schedulerAddActiveTask(&kernelTask);

    taskSwitching = 1;
}

static u32 getNextPid()
{
    static u32 last = 1;

    u32 i;
    for (i = last; i < TASK_MAX_PID; i++) {
        if (!tasksTable[i]) {
            last = i;
            return i;
        }
    }

    for (i = 1; i < last; i++) {
        if (!tasksTable[i]) {
            last = i;
            return i;
        }
    }

    return 0;
}

struct Task *createTask(struct TaskCreator *info)
{
    struct Task *task = kmalloc(sizeof(struct Task), 0, "task");
    u32 *stack = kmalloc(KERNEL_STACK_SIZE, 0, "stack");
    if (!task || !stack)
        return NULL;

    task->pid = getNextPid();
    tasksTable[task->pid] = task;

    task->type = info->type;
    task->privilege = info->privilege;
    task->parent = info->parent;
    listReset(&task->threads);
    listReset(&task->childs);

    task->pageDirectory = info->pageDirectory;
    stack = (void *) ((u32) stack + KERNEL_STACK_SIZE);
    task->kernelStack = stack;
    task->event.type = TaskEventNone;
    task->heap.start = USER_HEAP_START;
    task->heap.pos = task->heap.nbPage = 0;

    task->cmdline = info->cmdline;

    for (int i = 0; i < MAX_NB_KOBJECT; i++)
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

    *(--stack) = info->ac;          // eax. Used to give argc to user programs.
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

static const char **copyArrayIntoUserland(struct PageDirectory *pd, u32 position, u32 ac, const char **av)
{
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

    return (const char **) nnArgv;
}

/*
int forkProcess() {
    LOG("[TASK] fork start\n");
    struct Task *parrent = currentTask;

    struct Task *task = kmalloc(sizeof(struct Task), 0, "task");

    LOG("[TASK] create new stack\n");
    u32 *stack = kmalloc(KERNEL_STACK_SIZE, 0, "stack");
    if (!task || !stack)
        goto failure;

    memcpy(stack, currentTask->kernelStack - KERNEL_STACK_SIZE, KERNEL_STACK_SIZE);
    task->kernelStack = (void *) ((u32) stack + KERNEL_STACK_SIZE);

    LOG("stack = %u, kernelStack = %u, SIZE = %u\n", stack, task->kernelStack, KERNEL_STACK_SIZE);
    LOG("cur esp: %u, cur kerneltask: %u\n", currentTask->esp, currentTask->kernelStack);

    LOG("[TASK] duplicate page directory\n");
    task->pageDirectory = pagingDuplicatePageDirectory(currentTask->pageDirectory);
    if (task->pageDirectory == NULL)
        goto failure;

    LOG("[TASK] copy info from currentTask\n");
    if (currentTask->kernelStack > task->kernelStack) {
        task->esp = currentTask->esp - (currentTask->kernelStack - task->kernelStack);
        task->ss = currentTask->ss;
    } else {
        task->esp = currentTask->esp + (task->kernelStack - currentTask->kernelStack);
        task->ss = currentTask->ss;
    }

    task->event.type = TaskEventNone;
    task->pid = nextPid++;
    task->cmdline = strdup(currentTask->cmdline);
    task->currentDir = currentTask->currentDir;
    task->currentDir->refcount += 1;
    task->privilege = currentTask->privilege;

    task->console = currentTask->console;
    task->heap.start = currentTask->heap.start;
    task->heap.pos = currentTask->heap.pos;
    task->heap.nbPage = currentTask->heap.nbPage;

    for (int i = 0; i < MAX_NB_FILE; i++) {
        if (currentTask->objectList[i] != NULL)
            task->objectList[i] = koDupplicate(currentTask->objectList[i]);
        else
            task->objectList[i] = NULL;
    }

    schedulerAddTask(task);
    LOG("[TASK] fork end\n");

    if (currentTask == parrent) {
        LOG("papa\n");
        return task->pid;
    }
    LOG("child\n");
    return 0;

    failure:
    LOG("[TASK] fork failed\n");
    kfree(stack);
    kfree(task);
    return -1;
}*/

pid_t createProcess(const struct ExceveInfo *info)
{
    LOG("[TASK] openfile\n");
    struct FsPath *file = fsResolvePath(info->cmdline);
    if (!file) {
        klog("[TASK] Can not open file: %s\n", info->cmdline);
        return -ENOENT;
    }

    if (file->type == FS_FOLDER) {
        fsPathDestroy(file);
        return -EISDIR;
    }

    if ((file->mode & S_IXUSR) != S_IXUSR) {
        fsPathDestroy(file);
        return -EACCES;
    }

    s32 fileSize = file->size;

    LOG("[TASK] kmalloc of size %d\n", fileSize);
    char *data = kmalloc(sizeof(char) * fileSize, 0, "data bin file");
    if (data == NULL) {
        fsPathDestroy(file);
        klog("[TASK] Can not alloc memory\n");
        return -ENOMEM;
    }

    LOG("[TASK] read\n");
    s32 readSize = fsReadFile(file, data, (u32) fileSize, 0);
    fsPathDestroy(file);

    if (readSize != (s32) fileSize) {
        kfree(data);
        klog("[TASK] Can not read bin data: %d\n", readSize);
        return -EIO;
    }

    LOG("[TASK] alloc pagedirec\n");
    struct PageDirectory *pageDirectory = pagingCreatePageDirectory();
    if (pageDirectory == NULL) {
        kfree(data);
        klog("[TASK] Can not alloc new page directory\n");
        return -ENOMEM;
    }

    LOG("[TASK] loadbin\n");
    u32 entryPrg = loadBinary(pageDirectory, data, (u32) fileSize);
    kfree(data);

    if (entryPrg == 0) {
        klog("[TASK] Can not load binary: %s\n", info->cmdline);
        return -ENOEXEC;
    }

    struct Task *progParent = currentTask;
    if (currentTask->type == T_THREAD)
        progParent = currentTask->parent;

    u32 ac;
    for (ac = 0; info->av[ac] != NULL; ac++);
    u32 envc;
    for (envc = 0; info->env[envc] != NULL; envc++);

    pagingAlloc(pageDirectory, (void *) USER_STACK, (u32) USER_HEAP_START - (u32) USER_STACK, MEM_USER | MEM_WRITE);
    const char **newAv = copyArrayIntoUserland(pageDirectory, USER_ARG_BUFFER, ac, (const char **) info->av);
    const char **newEnv = copyArrayIntoUserland(pageDirectory, USER_ENV_BUFFER, envc, (const char **) info->env);

    LOG("[TASK] add task (entry: %X)\n", entryPrg);
    struct TaskCreator taskInfo = {
            .type = T_PROCESS,
            .pageDirectory = pageDirectory,
            .entryPoint = entryPrg,
            .privilege = TaskPrivilegeUser,
            .ac = ac,
            .av = newAv,
            .env = newEnv,
            .cmdline = strdup(info->cmdline),
            .parent = progParent
    };

    struct Task *task = createTask(&taskInfo);
    if (!task)
        return -ENOMEM;

    LOG("[TASK] adding task to parent\n");
    listAddElem(&progParent->childs, task);

    LOG("[TASK] setting data\n");
    task->currentDir = progParent->currentDir;
    task->currentDir->refcount++;

    task->rootDir = progParent->rootDir;
    task->rootDir->refcount++;

    struct Console *cons = consoleGetActiveConsole();

    if (info->fd_in == -1)
        task->objectList[0] = koCreate(KO_CONS_STD, cons, O_RDONLY);
    else {
        LOG("[TASK] redir stdin onto fd %d\n", info->fd_in);
        struct Kobject *tmp = taskGetKObjectByFd(info->fd_in);
        task->objectList[0] = koDupplicate(tmp);
    }

    if (info->fd_out == -1)
        task->objectList[1] = koCreate(KO_CONS_STD, cons, O_WRONLY);
    else {
        LOG("[TASK] redir stdout onto fd %d\n", info->fd_out);
        struct Kobject *tmp = taskGetKObjectByFd(info->fd_out);
        task->objectList[1] = koDupplicate(tmp);
    }


    task->objectList[2] = koCreate(KO_CONS_ERROR, cons, O_WRONLY);

    cons->activeProcess = task;
    task->console = cons;

    LOG("[TASK] push into scheduler\n");
    schedulerAddActiveTask(task);

    LOG("[TASK] end\n");
    return task->pid;
}

pid_t createThread(u32 entryPrg)
{
    LOG("[TASK] add task thread (entry: %X)\n", entryPrg);

    struct Task *procTask = currentTask;
    if (currentTask->type == T_THREAD)
        procTask = currentTask->parent;

    struct TaskCreator taskInfo = {
            .type = T_THREAD,
            .pageDirectory = procTask->pageDirectory,
            .entryPoint = entryPrg,
            .privilege = procTask->privilege,
            .ac = 0,
            .av = 0,
            .env = 0,
            .cmdline = procTask->cmdline,
            .parent = procTask
    };

    struct Task *task = createTask(&taskInfo);
    if (!task)
        return -ENOMEM;

    task->currentDir = procTask->currentDir;
    task->rootDir = procTask->rootDir;
    task->console = procTask->console;

    listAddElem(&procTask->threads, task);

    schedulerAddActiveTask(task);
    return task->pid;
}

int taskKill(struct Task *task)
{
    if (task == NULL)
        return -ESRCH;

    LOG("[TASK] Killing %u\n", task->pid);

    if (task->privilege == TaskPrivilegeKernel) {
        klog("[TASK] can not kill kernel tasks !!\n");
        return -EACCES;
    }

    char tmpTaskSwitching = taskSwitching;
    taskSwitching = 0;

    tasksTable[task->pid] = NULL;

    if (task == task->console->readingTask) {
        task->console->readingTask = NULL;
        mutexForceUnlock(&task->console->mtx);
    }

    if (task->type == T_PROCESS) {
        for (struct ListElem *tmp = task->threads.begin; tmp != NULL; tmp = tmp->next)
            taskKill(tmp->data);
        for (struct ListElem *tmp = task->childs.begin; tmp != NULL; tmp = tmp->next)
            taskKill(tmp->data);

        LOG("[TASK] kill process\n");
        for (int fd = 0; fd < MAX_NB_KOBJECT; fd++) {
            struct Kobject *obj = task->objectList[fd];
            if (obj)
                koDestroy(obj);
        }

        kfree((void *) task->cmdline);
        pagingDestroyPageDirectory(task->pageDirectory);

        LOG("[TASK] destroy path curDir\n");
        fsPathDestroy(task->currentDir);
        fsPathDestroy(task->rootDir);

        if (task->console->activeProcess == task) {
            task->console->activeProcess = task->parent;
            consoleSwitchVideoMode(task->console, ConsoleModeText);
        }

        LOG("[TASK] remove task from parent\n");
        listDeleteElem(&task->parent->childs, task);
    } else {
        LOG("[TASK] kill thread\n");
        listDeleteElem(&task->parent->threads, task);
    }

    kfree(task->kernelStack - KERNEL_STACK_SIZE);

    schedulerRemoveActiveTask(task);

    kfree(task);
    taskSwitching = tmpTaskSwitching;

    LOG("[TASK] kill end\n");

    if (currentTask == task) {
        schedulerForceSwitchTask();
    }

    return 0;
}

pid_t taskGetpid()
{
    return currentTask->pid;
}

int taskKillByPid(pid_t pid)
{
    struct Task *task = getTaskByPid(pid);
    if (task == NULL)
        return -ESRCH;

    return taskKill(task);
}

int taskExit()
{
    return taskKill(currentTask);
}

void taskWaitEvent(enum TaskEventType event, u32 arg)
{
    currentTask->event.type = event;
    currentTask->event.timer = gettick();
    currentTask->event.arg = arg;

    schedulerRemoveActiveTask(currentTask);
    schedulerAddWaitingTask(currentTask);
    schedulerForceSwitchTask();
}

void taskResetEvent(struct Task *task)
{
    if (task == NULL || task->event.type == TaskEventNone)
        return;

    task->event.type = TaskEventNone;
    schedulerRemoveWaitingTask(task);
    schedulerAddActiveTask(task);
}

void taskSaveState(u32 esp)
{
    currentTask->esp = esp;
}

u32 taskSwitch(struct Task *newTask)
{
    taskSwitching = 0;
    currentTask = newTask;

    switchTSS((u32) newTask->kernelStack, newTask->esp, newTask->ss);
    pagingSwitchPageDirectory(newTask->pageDirectory);

    taskSwitching = 1;
    // klog("Task switch to pid %u\n", newTask->pid);
    return newTask->esp;
}

u32 taskHeapInc(s32 inc)
{
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

int taskHeapSet(u32 addr)
{
    if (currentTask->pid == 0 || addr % PAGESIZE != 0)
        return -EFAULT;

    struct Heap *heap = &currentTask->heap;
    if (addr == heap->pos)
        return 0;

    return (taskHeapInc(addr - heap->pos) != 0 ? 0 : -ENOMEM);
}


struct Task *getTaskByPid(pid_t pid)
{
    if (pid > TASK_MAX_PID || pid < 0)
        return NULL;

    return tasksTable[pid];
}

int taskGetAvailableFd(struct Task *task)
{
    struct Kobject **tmp = task->objectList;
    if (task->type == T_THREAD)
        tmp = task->parent->objectList;

    int id;
    for (id = 0; id < MAX_NB_KOBJECT; id++)
        if (tmp[id] == NULL)
            break;

    if (id >= MAX_NB_KOBJECT)
        return -EMFILE;

    return id;
}

struct Kobject *taskGetKObjectByFd(int fd)
{
    if (fd >= MAX_NB_KOBJECT || fd < 0)
        return NULL;

    if (currentTask->type == T_THREAD)
        return currentTask->parent->objectList[fd];
    return currentTask->objectList[fd];
}

int taskSetKObjectByFd(int fd, struct Kobject *obj)
{
    if (fd >= MAX_NB_KOBJECT || fd < 0)
        return -EBADF;

    if (currentTask->type == T_THREAD)
        currentTask->parent->objectList[fd] = obj;
    else
        currentTask->objectList[fd] = obj;
    return 0;
}
