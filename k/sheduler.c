//
// Created by rebut_p on 15/12/18.
//

#include <include/stdio.h>
#include <include/cpu.h>
#include <sys/paging.h>
#include <io/pit.h>
#include <io/keyboard.h>
#include <include/list.h>
#include "sheduler.h"
#include "sys/console.h"

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
#define LOG(x, ...)

struct List taskLists = CREATE_LIST();

void schedulerAddTask(struct Task *task) {
    listAddElem(&taskLists, task);
}

void schedulerRemoveTask(struct Task *task) {
    listDeleteElem(&taskLists, task);
}

void schedulerDoNothing() {
    while (1)
        hlt();
}

static char checkTaskEvent(struct Task *task) {
    //LOG("Check task event for: %X\n", task);
    switch (task->event.type) {
        case TaskEventNone:
            return 1;
        case TaskEventTimer:
            if (gettick() - task->event.timer >= task->event.arg) {
                task->event.type = TaskEventNone;
                return 1;
            }
            return 0;
        case TaskEventWaitPid: {
            if (getTaskByPid(task->event.arg) == NULL) {
                task->event.type = TaskEventNone;
                return 1;
            }
            return 0;
        }
        case TaskEventKeyboard:
            if (isConsoleReadReady(task->console) == 1) {
                task->event.type = TaskEventNone;
                return 1;
            }
            return 0;
        default:
            return 0;
    }
}

static struct Task *schedulerGetNextTask() {
    u32 nbTask = listCountElem(&taskLists);

    for (u32 i = 0; i < nbTask; i++) {
        struct Task *newTask = listGetNextElem(&taskLists);
        if (checkTaskEvent(newTask))
            return newTask;
    }

    return freeTimeTask;
}

u32 schedulerSwitchTask(u32 esp) {
    taskSaveState(esp);

    struct Task *oldTask = currentTask;
    struct Task *newTask = schedulerGetNextTask();

    if (oldTask == newTask)
        return esp;
    LOG("Task switch: oldTask: %p / newTask: %p - pid %u %u %u\n", oldTask, newTask, newTask->pid, esp, newTask->esp);
    return taskSwitch(newTask);
}

void schedulerForceSwitchTask() {
    LOG("Scheduler: force switch task\n");
    asm volatile("int $126");
}

struct Task *schedulerGetTaskByIndex(u32 index) {
    return listGetElemByIndex(&taskLists, index);
}