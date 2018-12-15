//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_SHEDULER_H
#define KERNEL_SHEDULER_H

#include <k/types.h>
#include "task.h"

void schedulerDoNothing();

u32 schedulerSwitchTask(u32 esp);
void schedulerForceSwitchTask();

void schedulerAddTask(struct Task *task);
void schedulerRemoveTask(struct Task *task);

extern struct List taskLists;

#endif //KERNEL_SHEDULER_H
