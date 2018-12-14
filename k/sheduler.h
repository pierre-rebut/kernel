//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_SHEDULER_H
#define KERNEL_SHEDULER_H

#include <k/types.h>

u32 schedulerSwitchTask(u32 esp);
void schedulerForceSwitchTask();

#endif //KERNEL_SHEDULER_H
