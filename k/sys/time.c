//
// Created by rebut_p on 23/12/18.
//

#include "time.h"
#include <io/cmos.h>
#include <stdio.h>

#define BCDtoDecimal(x) (u8)((x >> 4) * 10 + (x & 0xF))

void cmosTime(struct tm *ptm) {
    ptm->second = BCDtoDecimal(cmosRead(CMOS_SECOND));
    ptm->minute = BCDtoDecimal(cmosRead(CMOS_MINUTE));
    ptm->hour = BCDtoDecimal(cmosRead(CMOS_HOUR));
    ptm->dayofmonth = BCDtoDecimal(cmosRead(CMOS_DAYOFMONTH));
    ptm->month = BCDtoDecimal(cmosRead(CMOS_MONTH));
    ptm->year = BCDtoDecimal(cmosRead(CMOS_YEAR));
    ptm->century = BCDtoDecimal(cmosRead(CMOS_CENTURY));
}

static const u16 days[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

static int isLeapyear(u16 year) {
    return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

static u8 calculateWeekday(u16 year, u8 month, int day) {
    day += 6;
    day += (year * 146097) / 400 + days[month - 1];

    if (isLeapyear(year) && (month < 2 || (month == 2 && day <= 28)))
        day--;

    return (day % 7 + 1);
}

static void writeInt(u16 val, char *dest) {
    if (val < 10)
        sprintf(dest, "0%u", val);
    else
        sprintf(dest, "%u", val);
}

static const char *const weekdays[] = {
        "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"
};

static const char *const months[] = {
        "Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin", "Juillet", "Auout", "Septembre", "Octobre",
        "Novembre", "Decembre"
};

int getCurrentDateAndTime(char *buf) {
    static struct tm pct = {.dayofmonth = 0xFF};
    static char dayofmonth[3];

    u8 temp = pct.dayofmonth;
    cmosTime(&pct);

    if (temp != pct.dayofmonth) {
        pct.weekday = calculateWeekday(100 * pct.century + pct.year, pct.month, pct.dayofmonth);

        writeInt(pct.dayofmonth, dayofmonth);
    }
    char hour[3], minute[3], second[3];
    writeInt(pct.hour, hour);
    writeInt(pct.minute, minute);
    writeInt(pct.second, second);

    int read = sprintf(buf, "%s %s %s %u, %s:%s:%s", weekdays[pct.weekday - 1], dayofmonth, months[pct.month - 1],
                        pct.century * 100 + pct.year, hour, minute, second);

    buf[read] = '\0';
    return read + 1;
}
