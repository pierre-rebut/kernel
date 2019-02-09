//
// Created by rebut_p on 15/12/18.
//

#include <include/kstdio.h>
#include <include/cpu.h>
#include <sys/paging.h>
#include <io/pit.h>
#include <io/keyboard.h>
#include <include/list.h>
#include "sheduler.h"
#include "sys/console.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static struct List activeTaskLists = CREATE_LIST();
static struct List activeTaskListsLowPriority = CREATE_LIST();
static struct List taskListsWaiting = CREATE_LIST();

void schedulerAddActiveTask(struct Task *task) {
    if (task->privilege == TaskPrivilegeKernel)
        listAddElem(&activeTaskLists, task);
    else
        listAddElem(&activeTaskListsLowPriority, task);
}

void schedulerRemoveActiveTask(struct Task *task) {
    if (task->privilege == TaskPrivilegeKernel)
        listDeleteElem(&activeTaskLists, task);
    else
        listDeleteElem(&activeTaskListsLowPriority, task);
}

void schedulerAddWaitingTask(struct Task *task) {
    listAddElem(&taskListsWaiting, task);
}

void schedulerRemoveWaitingTask(struct Task *task) {
    listDeleteElem(&taskListsWaiting, task);
}

void schedulerDoNothing() {
    while (1)
        hlt();
}

static void checkTaskEvent() {
    for (struct ListElem *tmp = taskListsWaiting.begin; tmp != NULL; tmp = tmp->next) {
        struct Task *task = tmp->data;

        LOG("[scheduler] Check task event for: %u\n", task->pid);
        switch (task->event.type) {
            case TaskEventTimer:
                if (gettick() - task->event.timer >= task->event.arg)
                    taskResetEvent(task);
                break;
            case TaskEventWaitPid:
                if (getTaskByPid(task->event.arg) == NULL)
                    taskResetEvent(task);
                break;
            default:;
        }
    }
}

static struct Task *schedulerGetNextTask() {
    checkTaskEvent();

    u32 nbTask = listCountElem(&activeTaskLists);
    if (nbTask > 0)
        return listGetNextElem(&activeTaskLists);

    nbTask = listCountElem(&activeTaskListsLowPriority);
    if (nbTask > 0)
        return listGetNextElem(&activeTaskListsLowPriority);

    return freeTimeTask;
}

u32 schedulerSwitchTask(u32 esp) {
    taskSaveState(esp);

    struct Task *oldTask = currentTask;
    struct Task *newTask = schedulerGetNextTask();

    if (oldTask == newTask)
        return esp;
    LOG("[scheduler] Task switch: oldTask: %p (pid %u, esp %u)/ newTask: %p (pid %u, esp %u)\n",
        oldTask, oldTask->pid, esp, newTask, newTask->pid, newTask->esp);
    return taskSwitch(newTask);
}

void schedulerForceSwitchTask() {
    LOG("[scheduler] force switch task\n");
    asm volatile("int $126");
}