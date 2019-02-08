//
// Created by rebut_p on 23/12/18.
//

#include "cmos.h"
#include "io.h"

#define CMOS_ADDRESS  0x70
#define CMOS_DATA     0x71

#define BIT_6_TO_0    0x7F

u8 cmosRead(u8 offset) {
    u8 tmp = inb(CMOS_ADDRESS) & (u8) (1 << 7);
    outb(CMOS_ADDRESS, (u8) (tmp | (offset & BIT_6_TO_0)));
    u8 retVal = inb(CMOS_DATA);
    return retVal;
}