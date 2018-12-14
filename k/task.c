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

static u32 nextPid = 1;

char taskSwitching = 0;
struct Task *lstTasks = NULL;
struct Task *currentTask = NULL;
struct Task kernelTask = {
        .pid = 0,
        .esp = 0,
        .ss = 0x10,
        .event = TaskEventNone,
        .heap = {0}
};

void initTasking() {
    kernelTask.pageDirectory = kernelPageDirectory;
    kernelTask.lstFiles[0] = createFileDescriptor(NULL, NULL, NULL, NULL);
    kernelTask.lstFiles[1] = createFileDescriptor(NULL, &writeTerminalFromFD, NULL, NULL);
    kernelTask.lstFiles[2] = createFileDescriptor(NULL, &writeSerialFromFD, NULL, NULL);

    currentTask = &kernelTask;
    taskSwitching = 1;
}

static struct Task *createTask(struct PageDirectory *pageDirectory, u32 entryPoint, u32 ac, char **av) {
    struct Task *task = kmalloc(sizeof(struct Task), 0);
    u32 *stack = kmalloc(KERNEL_STACK_SIZE, 4);
    if (!task || !stack)
        return NULL;

    task->pid = nextPid++;
    task->pageDirectory = pageDirectory;
    stack += KERNEL_STACK_SIZE;
    task->kernelStack = stack;
    task->event = TaskEventNone;
    task->heap.start = USER_STACK;
    task->heap.pos = task->heap.nbPage = 0;


    task->lstFiles[0] = createFileDescriptor(NULL, NULL, NULL, NULL);
    task->lstFiles[1] = createFileDescriptor(NULL, &writeTerminalFromFD, NULL, NULL);
    task->lstFiles[2] = createFileDescriptor(NULL, &writeSerialFromFD, NULL, NULL);

    pagingAlloc(pageDirectory, (void *) (USER_STACK - 10 * PAGESIZE), 10 * PAGESIZE, MEM_USER | MEM_WRITE);

    *(--stack) = 0x23;
    *(--stack) = USER_STACK;
    *(--stack) = 0x0202;

    *(--stack) = 0x18 | 0x3; // cs
    *(--stack) = entryPoint; // eip

    *(--stack) = 0;               // error code
    *(--stack) = 0;               // Interrupt nummer

    // General purpose registers w/o esp
    *(--stack) = ac;            // eax. Used to give argc to user programs.
    *(--stack) = (u32)av; // ecx. Used to give argv to user programs.
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;

    *(--stack) = 0x23;
    *(--stack) = 0x23;
    *(--stack) = 0x23;
    *(--stack) = 0x23;

    task->esp = (u32)stack;
    task->ss = 0x23;

    return task;
}

static void addTask(struct Task *task) {
    if (lstTasks == NULL) {
        task->prev = NULL;
        lstTasks = task;
    }
    else {
        struct Task *iter = lstTasks;
        while (iter->next != NULL)
            iter = iter->next;
        iter->next = task;
        task->prev = iter;
    }
    task->next = NULL;
}

int createProcess(const char *cmdline) {
    s32 fileSize = kfsLengthOfFile(cmdline);
    if (fileSize == 0) {
        kSerialPrintf("Can not get file info: %s\n", cmdline);
        return -1;
    }

    kSerialPrintf("task: openfile\n");
    int fd = open(cmdline, O_RDONLY);
    if (fd < 0) {
        kSerialPrintf("Can not open file: %s\n", cmdline);
        return -1;
    }

    kSerialPrintf("task: kmalloc\n");
    char *data = kmalloc(sizeof(char) * fileSize, 0);
    if (data == NULL) {
        close(fd);
        kSerialPrintf("Can not alloc memory\n");
        return -1;
    }

    kSerialPrintf("task: read\n");
    if (read(fd, data, (u32) fileSize) != (s32) fileSize) {
        kfree(data);
        close(fd);
        kSerialPrintf("Can not read bin data\n");
        return -1;
    }

    close(fd);
    kSerialPrintf("task: alloc pagedirec\n");
    struct PageDirectory *pageDirectory = pagingCreatePageDirectory();
    if (pageDirectory == NULL) {
        kfree(data);
        kSerialPrintf("Can not alloc new page directory\n");
        return -1;
    }

    kSerialPrintf("task: loadbin\n");
    u32 entryPrg = loadBinary(pageDirectory, data, (u32)fileSize);
    kfree(data);

    if (entryPrg == 0) {
        kSerialPrintf("Can not load binary: %s\n", cmdline);
        return -1;
    }

    kSerialPrintf("task: add task\n");
    struct Task *task = createTask(pageDirectory, entryPrg, 0, NULL);
    if (!task)
        return -1;

    addTask(task);

    kSerialPrintf("task: end\n");
    return 0;
}

int taskKill(struct Task *task) {
    kSerialPrintf("Killing task: %u\n", task->pid);

    if (task == &kernelTask) {
        kSerialPrintf("can not kill kernel task !!\n");
        return -1;
    }

    kSerialPrintf("test 0\n");

    char tmpTaskSwitching = taskSwitching;
    taskSwitching = 0;

    /* for (int fd = 0; fd < 255; fd++) {
        struct FileDescriptor *file = task->lstFiles[fd];;
        if (file) {
            kSerialPrintf("test file 0: %d\n", fd);
            if (file->closeFct) {
                kSerialPrintf("test file 1\n");
                //file->closeFct(file->entryData);
            }
            kSerialPrintf("test file 2\n");
            kfree(file);
        }
    }*/

    kSerialPrintf("test 1\n");

    kfree(task->kernelStack - KERNEL_STACK_SIZE);

    kSerialPrintf("test 2\n");

    pagingDestroyPageDirectory(task->pageDirectory);
    kSerialPrintf("test 3\n");

    if (task->next)
        task->next->prev = task->prev;

    if (task->prev)
        task->prev->next = task->next;

    kfree(task);
    taskSwitching = tmpTaskSwitching;

    if (currentTask == task) {
        schedulerForceSwitchTask();
    }

    return 0;
}

u32 taskGetpid() {
    return currentTask->pid;
}

int taskKillByPid(u32 pid) {
    for (struct Task *task = lstTasks; task != NULL; task = task->next){
        if (task->pid == pid) {
            taskKill(task);
            return pid;
        }
    }
    return -1;
}

int taskExit() {
    return taskKill(currentTask);
}

void taskAddEvent(enum TaskEvent event, u32 arg) {
    currentTask->event = event;
    currentTask->eventArg = arg;
    schedulerForceSwitchTask();
}

void taskSaveState(u32 esp) {
    currentTask->esp = esp;
}

u32 taskSwitch(struct Task *newTask) {
    taskSwitching = 0;
    currentTask = newTask;

    switchTSS((u32)newTask->kernelStack, newTask->esp, newTask->ss);
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
        if ((u32)inc > (heap->nbPage * PAGESIZE) - heap->pos) {
            u32 incPage = alignUp((u32)inc, PAGESIZE);
            pagingAlloc(currentTask->pageDirectory, (void *) (heap->start + heap->nbPage * PAGESIZE), incPage,
                        MEM_WRITE | MEM_USER);

            heap->nbPage += incPage / PAGESIZE;
        }
    } else {
        inc = -inc;
        if ((u32) inc < (heap->nbPage * PAGESIZE) - heap->pos) {
            u32 decPage = alignDown((u32)inc, PAGESIZE);
            u32 decPos = heap->start + heap->nbPage * PAGESIZE - decPage;
            pagingFree(currentTask->pageDirectory, (void*) decPos, decPage);
            heap->nbPage -= decPage / PAGESIZE;
        }

    }

    heap->pos += inc;
    return heap->start + brk;
}
