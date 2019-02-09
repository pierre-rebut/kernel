//
// Created by rebut_p on 24/12/18.
//

#include <include/kstdio.h>
#include "mutex.h"
#include "allocator.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

void mutexReset(struct Mutex *mtx) {
    mtx->locked = 0;
    mtx->currentTask = NULL;
    mtx->lstTasksLocked = NULL;
}

int mutexLock(struct Mutex *mtx) {
    LOG("[MTX] lock\n");
    char tmpTaskswitching = taskSwitching;
    taskSwitching = 0;

    if (mtx->currentTask == currentTask) {
        taskSwitching = tmpTaskswitching;
        LOG("[MTX] lock end 0\n");
        return 1;
    }

    if (mtx->locked == 0) {
        mtx->locked = 1;
        mtx->currentTask = currentTask;
        taskSwitching = tmpTaskswitching;
        LOG("[MTX] lock end 1\n");
        return 0;
    }

    struct MutexTaskLock *lock = kmalloc(sizeof(struct MutexTaskLock), 0, "newMtxTaskLock");
    if (lock == NULL) {
        taskSwitching = tmpTaskswitching;
        LOG("[MTX] lock failed\n");
        return -1;
    }

    lock->next = mtx->lstTasksLocked;
    mtx->lstTasksLocked = lock;
    taskSwitching = tmpTaskswitching;

    LOG("[MTX] lock end 2\n");
    taskWaitEvent(TaskEventMutex, 0);
    return 0;
}

int mutexTryLock(struct Mutex *mtx) {
    LOG("[MTX] trylock\n");
    char tmpTaskswitching = taskSwitching;
    taskSwitching = 0;

    int value = 0;
    if (mtx->currentTask == currentTask)
        value = 1;
    else if (mtx->locked == 0) {
        value = 1;
        mtx->locked = 1;
        mtx->currentTask = currentTask;
    }

    taskSwitching = tmpTaskswitching;
    LOG("[MTX] trylock end\n");
    return value;
}

int mutexForceUnlock(struct Mutex *mtx) {
    LOG("[MTX] unlock\n");
    char tmpTaskswitching = taskSwitching;
    taskSwitching = 0;

    if (mtx->lstTasksLocked != NULL) {
        struct MutexTaskLock *next = mtx->lstTasksLocked;
        mtx->lstTasksLocked = mtx->lstTasksLocked->next;

        mtx->currentTask = next->task;
        kfree(next);
        mtx->currentTask->event.type = TaskEventNone;
    } else {
        mtx->currentTask = NULL;
        mtx->locked = 0;
    }

    taskSwitching = tmpTaskswitching;
    LOG("[MTX] unlock end\n");
    return 0;
}

int mutexUnlock(struct Mutex *mtx) {
    if (mtx->currentTask != currentTask) {
        LOG("[MTX] unlock failed\n");
        return -1;
    }

    return mutexForceUnlock(mtx);
}
