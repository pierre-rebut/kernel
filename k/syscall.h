//
// Created by rebut_p on 23/09/18.
//

#ifndef KERNEL_EPITA_SYSCALL_H
#define KERNEL_EPITA_SYSCALL_H

#include "idt.h"

void syscall_handler(struct idt_context *ctx);

#endif //KERNEL_EPITA_SYSCALL_H
