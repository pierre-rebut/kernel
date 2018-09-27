//
// Created by rebut_p on 28/09/18.
//

#ifndef KERNEL_EPITA_BINARY_H
#define KERNEL_EPITA_BINARY_H

#include <k/types.h>
#include "multiboot.h"

int loadBinary(const module_t *module, u32 cmdline);

#endif //KERNEL_EPITA_BINARY_H
