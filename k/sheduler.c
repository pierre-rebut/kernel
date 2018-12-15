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
    //kSerialPrintf("putain de merde: %X\n", task);
    switch (task->event.type) {
        case TaskEventTimer:
            if (gettick() - task->event.timer >= task->event.arg) {
                task->event.type = TaskEventNone;
                return 1;
            }
            return 0;
        case TaskEventWaitPid: {
            struct Task *waiting = getTaskByPid(task->event.arg);
            if (waiting == NULL) {
                task->event.type = TaskEventNone;
                return 1;
            }
            return 0;
        }
        case TaskEventKeyboard:
            if (isKeyboardReady() == 1) {
                task->event.type = TaskEventNone;
                return 1;
            }
            return 0;
        default:
            return 1;
    }
}

static struct Task *schedulerGetNextTask() {
    u32 nbTask = listCountElem(&taskLists);

    for (u32 i = 0; i < nbTask; i++) {
        struct Task *newTask = listGetNextElem(&taskLists);
        if (checkTaskEvent(newTask))
            return newTask;
    }

    return kernelTask;
}

u32 schedulerSwitchTask(u32 esp) {
    taskSaveState(esp);

    struct Task *oldTask = currentTask;
    struct Task *newTask = schedulerGetNextTask();

    // kSerialPrintf("oldTask: %p / newTask: %p - pid %d %u %u\n", oldTask, newTask, newTask->pid, esp, newTask->esp);

    if (oldTask == newTask)
        return esp;

    return taskSwitch(newTask);
}

void schedulerForceSwitchTask() {
    // kSerialPrintf("Scheduler: force switch task\n");
    asm volatile("int $126");
}
