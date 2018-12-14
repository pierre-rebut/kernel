//
// Created by rebut_p on 28/09/18.
//

#include "include/multiboot.h"

#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#include <k/types.h>
#include <k/kstd.h>

void launchTask();
int createTask(const char *cmdline);
u32 task_switch(u32 previousEsp);
u32 sbrk(s32 inc);

#endif //KERNEL_EPITA_USERLAND_H
