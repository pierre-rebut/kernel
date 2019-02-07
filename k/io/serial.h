#ifndef KERNEL_EPITA_SERIAL_H
# define KERNEL_EPITA_SERIAL_H

#include <k/types.h>

void initSerial(int);
int writeSerial(const void *str, u32 length);

#endif //KERNEL_EPITA_SERIAL_H
