//
// Created by rebut_p on 23/12/18.
//

#ifndef KERNEL_EPITA_TIME_H
#define KERNEL_EPITA_TIME_H

#include <stddef.h>

struct tm
{
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

int getCurrentDateAndTime(char *buf, u32 size);

time_t mktime(struct tm *ptm);

#endif //KERNEL_EPITA_TIME_H
