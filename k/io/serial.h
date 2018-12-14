#ifndef KERNEL_EPITA_SERIAL_H
# define KERNEL_EPITA_SERIAL_H

#include <k/types.h>

void initSerial(int);
int writeSerial(const void *str, u32 length);

int writeSerialFromFD(void *tmp, void *data, u32 size);

#endif //KERNEL_EPITA_SERIAL_H
