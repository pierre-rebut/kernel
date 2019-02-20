//
// Created by rebut_p on 17/02/19.
//

#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#define INVALID    1
#define TOOSMALL    2
#define TOOLARGE    3

long long int strtoll(const char *nptr, char **endptr, int base)
{
    const char *s;
    /* LONGLONG */
    long long int acc, cutoff;
    int c;
    int neg, any, cutlim;

    /* endptr may be NULL */

#ifdef __GNUC__
    /* This outrageous construct just to shut up a GCC warning. */
    (void) &acc;
    (void) &cutoff;
#endif

    /*
     * Skip white space and pick up leading +/- sign if any.
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    s = nptr;
    do {
        c = (unsigned char) *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else {
        neg = 0;
        if (c == '+')
            c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    /*
     * Compute the cutoff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * base.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for long longs is
     * [-9223372036854775808..9223372036854775807] and the input base
     * is 10, cutoff will be set to 922337203685477580 and cutlim to
     * either 7 (neg==0) or 8 (neg==1), meaning that if we have
     * accumulated a value > 922337203685477580, or equal but the
     * next digit is > 7 (or 8), the number is too big, and we will
     * return a range error.
     *
     * Set any if any `digits' consumed; make it negative to indicate
     * overflow.
     */
    cutoff = neg ? LLONG_MIN : LLONG_MAX;
    cutlim = (int) (cutoff % base);
    cutoff /= base;
    if (neg) {
        if (cutlim > 0) {
            cutlim -= base;
            cutoff += 1;
        }
        cutlim = -cutlim;
    }
    for (acc = 0, any = 0;; c = (unsigned char) *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0)
            continue;
        if (neg) {
            if (acc < cutoff || (acc == cutoff && c > cutlim)) {
                any = -1;
                acc = LLONG_MIN;
                errno = ERANGE;
            } else {
                any = 1;
                acc *= base;
                acc -= c;
            }
        } else {
            if (acc > cutoff || (acc == cutoff && c > cutlim)) {
                any = -1;
                acc = LLONG_MAX;
                errno = ERANGE;
            } else {
                any = 1;
                acc *= base;
                acc += c;
            }
        }
    }
    if (endptr != 0)
        /* LINTED interface specification */
        *endptr = (char *) (any ? s - 1 : nptr);
    return (acc);
}

long long strtonum(const char *numstr, long long minval, long long maxval, const char **errstrp)
{
    long long ll = 0;
    char *ep;
    int error = 0;
    struct errval
    {
        const char *errstr;
        int err;
    } ev[4] = {
            {NULL, 0},
            {"invalid",   EINVAL},
            {"too small", ERANGE},
            {"too large", ERANGE},
    };

    ev[0].err = errno;
    errno = 0;
    if (minval > maxval)
        error = INVALID;
    else {
        ll = strtoll(numstr, &ep, 10);
        if (numstr == ep || *ep != '\0')
            error = INVALID;
        else if ((ll == LLONG_MIN && errno == ERANGE) || ll < minval)
            error = TOOSMALL;
        else if ((ll == LLONG_MAX && errno == ERANGE) || ll > maxval)
            error = TOOLARGE;
    }
    if (errstrp != NULL)
        *errstrp = ev[error].errstr;
    errno = ev[error].err;
    if (error)
        ll = 0;

    return (ll);
}