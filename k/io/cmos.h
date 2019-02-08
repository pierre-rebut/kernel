//
// Created by rebut_p on 23/12/18.
//

#ifndef KERNEL_CMOS_H
#define KERNEL_CMOS_H

#include <k/types.h>

#define CMOS_SECOND      0x00
#define CMOS_ALARMSECOND 0x01
#define CMOS_MINUTE      0x02
#define CMOS_ALARMMINUTE 0x03
#define CMOS_HOUR        0x04
#define CMOS_ALARMHOUR   0x05
#define CMOS_WEEKDAY     0x06
#define CMOS_DAYOFMONTH  0x07
#define CMOS_MONTH       0x08
#define CMOS_YEAR        0x09
#define CMOS_CENTURY     0x32

u8 cmosRead(u8 offset);

#endif //KERNEL_CMOS_H
