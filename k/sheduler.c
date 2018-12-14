//
// Created by rebut_p on 15/12/18.
//

#include "sheduler.h"
#include "task.h"

static char checkTaskEvent(struct Task *task) {
    if (task->event == TaskEventNone)
        return 1;

    return 0;
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

    return &kernelTask;
}

u32 schedulerSwitchTask(u32 esp) {
    taskSaveState(esp);

    struct Task *oldTask = currentTask;
    struct Task *newTask = schedulerGetNextTask();

    if (oldTask == newTask)
        return esp;

    return taskSwitch(newTask);
}

void schedulerForceSwitchTask() {
    asm volatile("int $126");
}
