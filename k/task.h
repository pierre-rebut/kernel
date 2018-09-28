//
// Created by rebut_p on 28/09/18.
//

#include "multiboot.h"

#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#include <k/types.h>

void createTask(u32 entry, u32 esp);
u32 task_switch(u32 previousEsp);

#endif //KERNEL_EPITA_USERLAND_H
