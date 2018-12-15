//
// Created by rebut_p on 15/12/18.
//

#include <include/stdio.h>
#include <include/cpu.h>
#include <sys/paging.h>
#include <io/pit.h>
#include "sheduler.h"
#include "task.h"

struct Task *freeTimeTask = NULL;

void schedulerDoNothing() {
    while (1)
        hlt();
}

static char checkTaskEvent(struct Task *task) {
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
        default:
            return 1;
    }
}

static struct Task *schedulerGetNextTask() {
    char tmp = 1;
    struct Task *newTask = currentTask->next;

    while (newTask != NULL || tmp) {
        if (newTask) {
            if (checkTaskEvent(newTask))
                return newTask;

            newTask = newTask->next;
        }

        if (newTask == NULL && tmp) {
            tmp = 0;
            newTask = lstTasks;
        }
    }

    if (!freeTimeTask)
        freeTimeTask = createTask(kernelPageDirectory, (u32) (&schedulerDoNothing), TaskPrivilegeKernel, 0, NULL, NULL);

    return freeTimeTask;
}

u32 schedulerSwitchTask(u32 esp) {
    taskSaveState(esp);

    struct Task *oldTask = currentTask;
    struct Task *newTask = schedulerGetNextTask();

    // kSerialPrintf("oldTask: %p / newTask: %p - pid %d\n", oldTask, newTask, newTask->pid);

    if (oldTask == newTask)
        return esp;

    return taskSwitch(newTask);
}

void schedulerForceSwitchTask() {
    kSerialPrintf("Scheduler: force switch task\n");
    asm volatile("int $126");
}
