#ifndef KERNEL_EPITA_TIME_H
#define KERNEL_EPITA_TIME_H

#include <k/types.h>

struct tm {
    u8 second;
    u8 minute;
    u8 hour;
    u8 weekday;
    u8 dayofmonth;
    u8 month;
    u8 year;
    u8 century;
};

void cmosTime(struct tm *ptm);
int getCurrentDateAndTime(char *pStr);

#endif //KERNEL_EPITA_TIME_H
