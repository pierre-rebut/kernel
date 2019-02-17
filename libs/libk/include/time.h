//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <stddef.h>

#define TM_YEAR_BASE 1970

struct tm
{
  int tm_sec;                   /* Seconds.     [0-60] (1 leap second) */
  int tm_min;                   /* Minutes.     [0-59] */
  int tm_hour;                  /* Hours.       [0-23] */
  int tm_mday;                  /* Day.         [1-31] */
  int tm_mon;                   /* Month.       [0-11] */
  int tm_year;                  /* Year - 1900.  */
  int tm_wday;                  /* Day of week. [0-6] */
  int tm_yday;                  /* Days in year.[0-365] */
  int tm_isdst;                 /* DST.         [-1/0/1]*/

  long int tm_gmtoff;           /* Seconds east of UTC.  */
  const char *tm_zone;          /* Timezone abbreviation.  */
};

time_t mktime (struct tm *__tp);
time_t time(time_t *tloc);
size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *t);
char *ctime(const time_t *timer);
char *asctime(const struct tm *tm);
struct tm *localtime(const time_t *timer);
struct tm *gmtime(const time_t *timer);

const char *getMonthAbrev(int mon);

#endif //KERNEL_TIME_H
