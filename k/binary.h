//
// Created by rebut_p on 28/09/18.
//

#ifndef KERNEL_EPITA_BINARY_H
#define KERNEL_EPITA_BINARY_H

#include <k/types.h>
#include "multiboot.h"
#include "paging.h"

u32 loadBinary(struct PageDirectory *pd, const void *data, u32 size);

#endif //KERNEL_EPITA_BINARY_H
