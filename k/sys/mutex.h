//
// Created by rebut_p on 24/12/18.
//

#ifndef KERNEL_MUTEX_H
#define KERNEL_MUTEX_H

#include "task.h"

struct MutexTaskLock {
    struct Task *task;
    struct MutexTaskLock *next;
};

struct Mutex {
    int locked;
    struct Task *currentTask;
    struct MutexTaskLock *lstTasksLocked;
};

#define mutexInit() {0, NULL, NULL}
void mutexReset(struct Mutex *mtx);
int mutexLock(struct Mutex *mtx);
int mutexTryLock(struct Mutex *mtx);
int mutexUnlock(struct Mutex *mtx);

#endif //KERNEL_MUTEX_H
