//
// Created by rebut_p on 17/02/19.
//

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <utils.h>
#include <stdlib.h>
#include <alloc.h>

static const int maxscale = 7;

int humanize_number(char *buf, size_t len, long long quotient, const char *suffix, int scale, int flags) {
    const char *prefixes, *sep;
    int i, r, remainder, s1, s2, sign;
    int divisordeccut;
    long long divisor, max;
    size_t baselen;

    /* Since so many callers don't check -1, NUL terminate the buffer */
    if (len > 0)
        buf[0] = '\0';

    /* validate args */
    if (buf == NULL || suffix == NULL)
        return (-1);
    if (scale < 0)
        return (-1);
    else if (scale >= maxscale &&
             ((scale & ~(HN_AUTOSCALE | HN_GETSCALE)) != 0))
        return (-1);
    if ((flags & HN_DIVISOR_1000) && (flags & HN_IEC_PREFIXES))
        return (-1);

    /* setup parameters */
    remainder = 0;

    if (flags & HN_IEC_PREFIXES) {
        baselen = 2;
        /*
         * Use the prefixes for power of two recommended by
         * the International Electrotechnical Commission
         * (IEC) in IEC 80000-3 (i.e. Ki, Mi, Gi...).
         *
         * HN_IEC_PREFIXES implies a divisor of 1024 here
         * (use of HN_DIVISOR_1000 would have triggered
         * an assertion earlier).
         */
        divisor = 1024;
        divisordeccut = 973;    /* ceil(.95 * 1024) */
        if (flags & HN_B)
            prefixes = "B\0\0Ki\0Mi\0Gi\0Ti\0Pi\0Ei";
        else
            prefixes = "\0\0\0Ki\0Mi\0Gi\0Ti\0Pi\0Ei";
    } else {
        baselen = 1;
        if (flags & HN_DIVISOR_1000) {
            divisor = 1000;
            divisordeccut = 950;
            if (flags & HN_B)
                prefixes = "B\0\0k\0\0M\0\0G\0\0T\0\0P\0\0E";
            else
                prefixes = "\0\0\0k\0\0M\0\0G\0\0T\0\0P\0\0E";
        } else {
            divisor = 1024;
            divisordeccut = 973;    /* ceil(.95 * 1024) */
            if (flags & HN_B)
                prefixes = "B\0\0K\0\0M\0\0G\0\0T\0\0P\0\0E";
            else
                prefixes = "\0\0\0K\0\0M\0\0G\0\0T\0\0P\0\0E";
        }
    }

#define    SCALE2PREFIX(scale)    (&prefixes[(scale) * 3])

    if (quotient < 0) {
        sign = -1;
        quotient = -quotient;
        baselen += 2;        /* sign, digit */
    } else {
        sign = 1;
        baselen += 1;        /* digit */
    }
    if (flags & HN_NOSPACE)
        sep = "";
    else {
        sep = " ";
        baselen++;
    }
    baselen += strlen(suffix);

    /* Check if enough room for `x y' + suffix + `\0' */
    if (len < baselen + 1)
        return (-1);

    if (scale & (HN_AUTOSCALE | HN_GETSCALE)) {
        /* See if there is additional columns can be used. */
        for (max = 1, i = len - baselen; i-- > 0 && max <= (2147483647 / 10);)
            max *= 10;

        /*
         * Divide the number until it fits the given column.
         * If there will be an overflow by the rounding below,
         * divide once more.
         */
        for (i = 0;
             (quotient >= max || (quotient == max - 1 &&
                                  remainder >= divisordeccut)) && i < maxscale; i++) {
            remainder = quotient % divisor;
            quotient /= divisor;
        }

        if (scale & HN_GETSCALE)
            return (i);
    } else {
        for (i = 0; i < scale && i < maxscale; i++) {
            remainder = quotient % divisor;
            quotient /= divisor;
        }
    }

    /* If a value <= 9.9 after rounding and ... */
    /*
     * XXX - should we make sure there is enough space for the decimal
     * place and if not, don't do HN_DECIMAL?
     */
    if (((quotient == 9 && remainder < divisordeccut) || quotient < 9) &&
        i > 0 && flags & HN_DECIMAL) {
        s1 = (int) quotient + ((remainder * 10 + divisor / 2) /
                               divisor / 10);
        s2 = ((remainder * 10 + divisor / 2) / divisor) % 10;
        r = snprintf(buf, len, "%d.%d%s%s%s", sign * s1, s2, sep, SCALE2PREFIX(i), suffix);
    } else
        r = snprintf(buf, len, "%us%s%s", sign * (quotient + (remainder + divisor / 2) / divisor),
                     sep, SCALE2PREFIX(i), suffix);

    return (r);
}
/*
static struct {
    char *name;
    u_long flag;
    int invert;
} mapping[] = {

        {"nosappnd",     SF_APPEND,     0},
        {"nosappend",    SF_APPEND,     0},
        {"noarch",       SF_ARCHIVED,   0},
        {"noarchived",   SF_ARCHIVED,   0},
        {"noschg",       SF_IMMUTABLE,  0},
        {"noschange",    SF_IMMUTABLE,  0},
        {"nosimmutable", SF_IMMUTABLE,  0},
        {"nouappnd",     UF_APPEND,     0},
        {"nouappend",    UF_APPEND,     0},
        {"nouchg",       UF_IMMUTABLE,  0},
        {"nouchange",    UF_IMMUTABLE,  0},
        {"nouimmutable", UF_IMMUTABLE,  0},
        {"nodump",       UF_NODUMP,     1},
        {"noopaque",     UF_OPAQUE,     0},
        {"nohidden",     UF_HIDDEN,     0},
        {"nocompressed", UF_COMPRESSED, 0},
};*/
#define longestflaglen    12
#define nmappings    (sizeof(mapping) / sizeof(mapping[0]))

/*
 * fflagstostr --
 *	Convert file flags to a comma-separated string.  If no flags
 *	are set, return the empty string.
 */
char *fflagstostr(u_long flags) {
    /*char *string;
    char *sp, *dp;
    u_long setflags;
    int i;

    if ((string = (char *) malloc(nmappings * (longestflaglen + 1))) == NULL)
        return (NULL);

    setflags = flags;
    dp = string;
    for (i = 0; i < nmappings; i++) {
        if (setflags & mapping[i].flag) {
            if (dp > string)
                *dp++ = ',';
            for (sp = mapping[i].invert ? mapping[i].name :
                      mapping[i].name + 2; *sp; *dp++ = *sp++);
            setflags &= ~mapping[i].flag;
        }
    }*/
    (void) flags;
    char *dp = malloc(1);
    *dp = '\0';
    return (dp);
}

/*
 * strtofflags --
 *	Take string of arguments and return file flags.  Return 0 on
 *	success, 1 on failure.  On failure, stringp is set to point
 *	to the offending token.
 *
int strtofflags(char **stringp, u_long *setp, u_long *clrp) {
    char *string, *p;
    int i;

    if (setp)
        *setp = 0;
    if (clrp)
        *clrp = 0;
    string = *stringp;
    while ((p = strsep(&string, "\t ,")) != NULL) {
        *stringp = p;
        if (*p == '\0')
            continue;
        for (i = 0; i < nmappings; i++) {
            if (strcmp(p, mapping[i].name + 2) == 0) {
                if (mapping[i].invert) {
                    if (clrp)
                        *clrp |= mapping[i].flag;
                } else {
                    if (setp)
                        *setp |= mapping[i].flag;
                }
                break;
            } else if (strcmp(p, mapping[i].name) == 0) {
                if (mapping[i].invert) {
                    if (setp)
                        *setp |= mapping[i].flag;
                } else {
                    if (clrp)
                        *clrp |= mapping[i].flag;
                }
                break;
            }
        }
        if (i == nmappings)
            return 1;
    }
    return 0;
}*/
