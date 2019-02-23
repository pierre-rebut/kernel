//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_SHEDULER_H
#define KERNEL_SHEDULER_H

#include <stddef.h>
#include "task.h"

void schedulerDoNothing();

u32 schedulerSwitchTask(u32 esp);

void schedulerForceSwitchTask();

void schedulerAddActiveTask(struct Task *task);

void schedulerRemoveActiveTask(struct Task *task);

void schedulerAddWaitingTask(struct Task *task);

void schedulerRemoveWaitingTask(struct Task *task);

#endif //KERNEL_SHEDULER_H
