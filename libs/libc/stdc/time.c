//
// Created by rebut_p on 17/02/19.
//

#include <time.h>
#include "syscalls.h"

#define YEAR0                   1900
#define EPOCH_YR                1970
#define SECS_DAY                (24L * 60L * 60L)
#define LEAPYEAR(year)          (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)          (LEAPYEAR(year) ? 366 : 365)
#define FIRSTSUNDAY(timp)       (((timp)->tm_yday - (timp)->tm_wday + 420) % 7)
#define FIRSTDAYOF(timp)        (((timp)->tm_wday - (timp)->tm_yday + 420) % 7)

#define TIME_MAX                2147483647L

//static int _daylight = 0;                  // Non-zero if daylight savings time is used
static long _dstbias = 0;                  // Offset for Daylight Saving Time
static long _timezone = 0;                 // Difference in seconds between GMT and local time
//static char *_tzname[2] = {"GMT", "GMT"};  // Standard/daylight savings time zone names

static const char *_days[] = {
        "Dimanche", "Lundi", "Mardi", "Mercredi",
        "Jeudi", "Vendredi", "Samedi"
};

static const char *_days_abbrev[] = {
        "Dim", "Lun", "Mar", "Mer",
        "Jeu", "Ven", "Sam"
};

static const char *_months[] = {
        "Janvier", "Fevrier", "Mars",
        "Avril", "Mai", "Juin",
        "JUillet", "Aout", "Septembre",
        "Octobre", "Novembre", "Decembre"
};

static const char *_months_abbrev[] = {
        "Jan", "Fev", "Mar",
        "Avr", "Mai", "Jui",
        "Jul", "Aou", "Sep",
        "Oct", "Nov", "Dec"
};

static const int _ytab[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

time_t time(time_t *tloc)
{
    int res = syscall1(SYSCALL_TIME, (u32) tloc);
    if (res < 0)
        return 0;
    return (u32) res;
}

const char *getMonthAbrev(int mon)
{
    if (mon < 0 || mon > 11)
        return NULL;
    return _months_abbrev[mon];
}

static struct tm *gmtime_r(const time_t *timer, struct tm *tmbuf)
{
    time_t time = *timer;
    unsigned long dayclock, dayno;
    int year = EPOCH_YR;

    dayclock = (unsigned long) time % SECS_DAY;
    dayno = (unsigned long) time / SECS_DAY;

    tmbuf->tm_sec = dayclock % 60;
    tmbuf->tm_min = (dayclock % 3600) / 60;
    tmbuf->tm_hour = dayclock / 3600;
    tmbuf->tm_wday = (dayno + 4) % 7; // Day 0 was a thursday
    while (dayno >= (unsigned long) YEARSIZE(year)) {
        dayno -= YEARSIZE(year);
        year++;
    }
    tmbuf->tm_year = year - YEAR0;
    tmbuf->tm_yday = dayno;
    tmbuf->tm_mon = 0;
    while (dayno >= (unsigned long) _ytab[LEAPYEAR(year)][tmbuf->tm_mon]) {
        dayno -= _ytab[LEAPYEAR(year)][tmbuf->tm_mon];
        tmbuf->tm_mon++;
    }
    tmbuf->tm_mday = dayno + 1;
    tmbuf->tm_isdst = 0;
    tmbuf->tm_gmtoff = 0;
    tmbuf->tm_zone = "UTC";
    return tmbuf;
}

static struct tm *localtime_r(const time_t *timer, struct tm *tmbuf)
{
    time_t t;

    t = *timer - _timezone;
    return gmtime_r(&t, tmbuf);
}

#ifndef KERNEL

struct tm *gmtime(const time_t *timer)
{
    static struct tm buf;
    return gmtime_r(timer, &buf);
}

struct tm *localtime(const time_t *timer)
{
    static struct tm buf;
    return localtime_r(timer, &buf);
}

#endif

time_t mktime(struct tm *tmbuf)
{
    long day, year;
    int tm_year;
    int yday, month;
    /*unsigned*/ long seconds;
    int overflow;
    long dst;

    tmbuf->tm_min += tmbuf->tm_sec / 60;
    tmbuf->tm_sec %= 60;
    if (tmbuf->tm_sec < 0) {
        tmbuf->tm_sec += 60;
        tmbuf->tm_min--;
    }
    tmbuf->tm_hour += tmbuf->tm_min / 60;
    tmbuf->tm_min = tmbuf->tm_min % 60;
    if (tmbuf->tm_min < 0) {
        tmbuf->tm_min += 60;
        tmbuf->tm_hour--;
    }
    day = tmbuf->tm_hour / 24;
    tmbuf->tm_hour = tmbuf->tm_hour % 24;
    if (tmbuf->tm_hour < 0) {
        tmbuf->tm_hour += 24;
        day--;
    }
    tmbuf->tm_year += tmbuf->tm_mon / 12;
    tmbuf->tm_mon %= 12;
    if (tmbuf->tm_mon < 0) {
        tmbuf->tm_mon += 12;
        tmbuf->tm_year--;
    }
    day += (tmbuf->tm_mday - 1);
    while (day < 0) {
        if (--tmbuf->tm_mon < 0) {
            tmbuf->tm_year--;
            tmbuf->tm_mon = 11;
        }
        day += _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
    }
    while (day >= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon]) {
        day -= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
        if (++(tmbuf->tm_mon) == 12) {
            tmbuf->tm_mon = 0;
            tmbuf->tm_year++;
        }
    }
    tmbuf->tm_mday = day + 1;
    year = EPOCH_YR;
    if (tmbuf->tm_year < year - YEAR0) return (time_t) -1;
    seconds = 0;
    day = 0;                      // Means days since day 0 now
    overflow = 0;

    // Assume that when day becomes negative, there will certainly
    // be overflow on seconds.
    // The check for overflow needs not to be done for leapyears
    // divisible by 400.
    // The code only works when year (1970) is not a leapyear.
    tm_year = tmbuf->tm_year + YEAR0;

    if (TIME_MAX / 365 < tm_year - year) overflow++;
    day = (tm_year - year) * 365;
    if (TIME_MAX - day < (tm_year - year) / 4 + 1) overflow++;
    day += (tm_year - year) / 4 + ((tm_year % 4) && tm_year % 4 < year % 4);
    day -= (tm_year - year) / 100 + ((tm_year % 100) && tm_year % 100 < year % 100);
    day += (tm_year - year) / 400 + ((tm_year % 400) && tm_year % 400 < year % 400);

    yday = month = 0;
    while (month < tmbuf->tm_mon) {
        yday += _ytab[LEAPYEAR(tm_year)][month];
        month++;
    }
    yday += (tmbuf->tm_mday - 1);
    if (day + yday < 0) overflow++;
    day += yday;

    tmbuf->tm_yday = yday;
    tmbuf->tm_wday = (day + 4) % 7;               // Day 0 was thursday (4)

    seconds = ((tmbuf->tm_hour * 60L) + tmbuf->tm_min) * 60L + tmbuf->tm_sec;

    if ((TIME_MAX - seconds) / SECS_DAY < day) overflow++;
    seconds += day * SECS_DAY;

    // Now adjust according to timezone and daylight saving time
    if (((_timezone > 0) && (TIME_MAX - _timezone < seconds)) ||
        ((_timezone < 0) && (seconds < -_timezone))) {
        overflow++;
    }
    seconds += _timezone;

    if (tmbuf->tm_isdst) {
        dst = _dstbias;
    } else {
        dst = 0;
    }

    if (dst > seconds) overflow++;        // dst is always non-negative
    seconds -= dst;

    if (overflow) return (time_t) -1;

    if ((time_t) seconds != seconds) return (time_t) -1;
    return (time_t) seconds;
}

char *asctime_r(const struct tm *tm, char *buf)
{
    strftime(buf, ASCBUFSIZE, "%c\n", tm);
    return buf;
}

char *ctime_r(const time_t *timer, char *buf)
{
    return asctime_r(localtime(timer), buf);
}

char *asctime(const struct tm *tm)
{
    static char buf[ASCBUFSIZE];
    return asctime_r(tm, buf);
}

char *ctime(const time_t *timer)
{
    return asctime(localtime(timer));
}

char *_strdate(char *s)
{
    time_t now;

    time(&now);
    strftime(s, 9, "%D", localtime(&now));
    return s;
}

char *_strtime(char *s)
{
    time_t now;

    time(&now);
    strftime(s, 9, "%T", localtime(&now));
    return s;
}

static size_t gsize;
static char *pt;

static int _add(char *);

static int _conv(int, int, int);

static int _secs(const struct tm *);

static size_t _fmt(const char *, const struct tm *);

size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *t)
{

    pt = s;
    if ((gsize = maxsize) < 1)
        return (0);
    if (_fmt(format, t)) {
        *pt = '\0';
        return (maxsize - gsize);
    }
    return (0);
}

static size_t _fmt(register const char *format, const struct tm *t)
{
    for (; *format; ++format) {
        if (*format == '%')
            switch (*++format) {
                case '\0':
                    --format;
                    break;
                case 'A':
                    if (t->tm_wday < 0 || t->tm_wday > 6)
                        return (0);
                    if (!_add((char *) _days[t->tm_wday]))
                        return (0);
                    continue;
                case 'a':
                    if (t->tm_wday < 0 || t->tm_wday > 6)
                        return (0);
                    if (!_add((char *) _days_abbrev[t->tm_wday]))
                        return (0);
                    continue;
                case 'B':
                    if (t->tm_mon < 0 || t->tm_mon > 11)
                        return (0);
                    if (!_add((char *) _months[t->tm_mon]))
                        return (0);
                    continue;
                case 'b':
                case 'h':
                    if (t->tm_mon < 0 || t->tm_mon > 11)
                        return (0);
                    if (!_add((char *) _months_abbrev[t->tm_mon]))
                        return (0);
                    continue;
                case 'C':
                    if (!_fmt("%a %b %e %H:%M:%S %Y", t))
                        return (0);
                    continue;
                case 'c':
                    if (!_fmt("%m/%d/%y %H:%M:%S", t))
                        return (0);
                    continue;
                case 'D':
                    if (!_fmt("%m/%d/%y", t))
                        return (0);
                    continue;
                case 'd':
                    if (!_conv(t->tm_mday, 2, '0'))
                        return (0);
                    continue;
                case 'e':
                    if (!_conv(t->tm_mday, 2, ' '))
                        return (0);
                    continue;
                case 'H':
                    if (!_conv(t->tm_hour, 2, '0'))
                        return (0);
                    continue;
                case 'I':
                    if (!_conv(t->tm_hour % 12 ?
                               t->tm_hour % 12 : 12, 2, '0'))
                        return (0);
                    continue;
                case 'j':
                    if (!_conv(t->tm_yday + 1, 3, '0'))
                        return (0);
                    continue;
                case 'k':
                    if (!_conv(t->tm_hour, 2, ' '))
                        return (0);
                    continue;
                case 'l':
                    if (!_conv(t->tm_hour % 12 ?
                               t->tm_hour % 12 : 12, 2, ' '))
                        return (0);
                    continue;
                case 'M':
                    if (!_conv(t->tm_min, 2, '0'))
                        return (0);
                    continue;
                case 'm':
                    if (!_conv(t->tm_mon + 1, 2, '0'))
                        return (0);
                    continue;
                case 'n':
                    if (!_add("\n"))
                        return (0);
                    continue;
                case 'p':
                    if (!_add(t->tm_hour >= 12 ? "PM" : "AM"))
                        return (0);
                    continue;
                case 'R':
                    if (!_fmt("%H:%M", t))
                        return (0);
                    continue;
                case 'r':
                    if (!_fmt("%I:%M:%S %p", t))
                        return (0);
                    continue;
                case 'S':
                    if (!_conv(t->tm_sec, 2, '0'))
                        return (0);
                    continue;
                case 's':
                    if (!_secs(t))
                        return (0);
                    continue;
                case 'T':
                case 'X':
                    if (!_fmt("%H:%M:%S", t))
                        return (0);
                    continue;
                case 't':
                    if (!_add("\t"))
                        return (0);
                    continue;
                case 'U':
                    if (!_conv((t->tm_yday + 7 - t->tm_wday) / 7,
                               2, '0'))
                        return (0);
                    continue;
                case 'W':
                    if (!_conv((t->tm_yday + 7 -
                                (t->tm_wday ? (t->tm_wday - 1) : 6))
                               / 7, 2, '0'))
                        return (0);
                    continue;
                case 'w':
                    if (!_conv(t->tm_wday, 1, '0'))
                        return (0);
                    continue;
                case 'x':
                    if (!_fmt("%m/%d/%y", t))
                        return (0);
                    continue;
                case 'y':
                    if (!_conv((t->tm_year + TM_YEAR_BASE)
                               % 100, 2, '0'))
                        return (0);
                    continue;
                case 'Y':
                    if (!_conv(t->tm_year + TM_YEAR_BASE, 4, '0'))
                        return (0);
                    continue;
                case 'Z':
                    if (!t->tm_zone || !_add((char *) t->tm_zone))
                        return (0);
                    continue;
                case '%':
                    /*
                     * X311J/88-090 (4.12.3.5): if conversion char is
                     * undefined, behavior is undefined.  Print out the
                     * character itself as printf(3) does.
                     */
                default:
                    break;
            }
        if (!gsize--)
            return (0);
        *pt++ = *format;
    }
    return (gsize);
}

static int _secs(const struct tm *t)
{
    static char buf[15];
    register time_t s;
    register char *p;
    struct tm tmp;

    /* Make a copy, mktime(3) modifies the tm struct. */
    tmp = *t;
    s = mktime(&tmp);
    for (p = buf + sizeof(buf) - 2; s > 0 && p > buf; s /= 10)
        *p-- = (char) (s % 10 + '0');
    return (_add(++p));
}

static int _conv(int n, int digits, int pad)
{
    static char buf[10];
    register char *p;

    for (p = buf + sizeof(buf) - 2; n > 0 && p > buf; n /= 10, --digits)
        *p-- = (char) (n % 10 + '0');
    while (p > buf && digits-- > 0)
        *p-- = (char) pad;
    return (_add(++p));
}

static int _add(register char *str)
{
    for (;; ++pt, --gsize) {
        if (!gsize)
            return (0);
        if (!(*pt = *str++))
            return (1);
    }
}