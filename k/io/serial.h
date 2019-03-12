//
// Created by rebut_p on 20/09/18.
//

#ifndef KERNEL_EPITA_SERIAL_H
# define KERNEL_EPITA_SERIAL_H

#include <stddef.h>

void initSerial(int);

int writeSerial(const void *str, u32 length);

#endif //KERNEL_EPITA_SERIAL_H
