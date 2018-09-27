//
// Created by rebut_p on 28/09/18.
//

#include "multiboot.h"

#ifndef KERNEL_EPITA_USERLAND_H
#define KERNEL_EPITA_USERLAND_H

#define USERCODE 0
#define USERDATA 1
#define USERSTACK 2
#define USERHEAP 3

#include <k/types.h>

void addUserlandEntry(u32 id, u32 base, u32 limit);

#endif //KERNEL_EPITA_USERLAND_H
