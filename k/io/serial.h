#ifndef KERNEL_EPITA_SERIAL_H
# define KERNEL_EPITA_SERIAL_H

#include <stddef.h>

void initSerial(int);
int writeSerial(const void *str, size_t length);

#endif //KERNEL_EPITA_SERIAL_H
