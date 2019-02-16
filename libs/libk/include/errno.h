//
// Created by rebut_p on 15/02/19.
//

#ifndef KERNEL_ERRNO_H
#define KERNEL_ERRNO_H

#include <types.h>
#include <errno-base.h>

extern int errno;
const char *strerror(u32 e);

#endif //KERNEL_ERRNO_H
