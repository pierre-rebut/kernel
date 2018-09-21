#ifndef COMDRIVER_H_
# define COMDRIVER_H_

#include <stddef.h>

void initSerial(int);
int write(const void *str, size_t length);

#endif
