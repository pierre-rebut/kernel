//
// Created by rebut_p on 21/12/18.
//

#include <string.h>
#include <stdlib.h>
#include <limits.h>

int atoi(const char *string) {
    int result = 0;
    unsigned int digit;
    int sign;

    while (isspace(*string)) {
        string += 1;
    }

    if (*string == '-') {
        sign = 1;
        string += 1;
    } else {
        sign = 0;
        if (*string == '+') {
            string += 1;
        }
    }

    for (;; string += 1) {
        digit = *string - '0';
        if (digit > 9) {
            break;
        }
        result = (10 * result) + digit;
    }

    if (sign) {
        return -result;
    }
    return result;
}

long atol(const char *string) {
    long int result = 0;
    unsigned int digit;
    int sign;

    while (isspace(*string)) {
        string += 1;
    }

    if (*string == '-') {
        sign = 1;
        string += 1;
    } else {
        sign = 0;
        if (*string == '+') {
            string += 1;
        }
    }

    for (;; string += 1) {
        digit = *string - '0';
        if (digit > 9) {
            break;
        }
        result = (10 * result) + digit;
    }

    if (sign) {
        return -result;
    }
    return result;
}

static int maxExponent = 511;    /* Largest possible base 10 exponent.  Any
				 * exponent larger than this will already
				 * produce underflow or overflow, so there's
				 * no need to worry about additional digits.
				 */
static double powersOf10[] = {    /* Table giving binary powers of 10.  Entry */
        10.,            /* is 10^2^i.  Used to convert decimal */
        100.,            /* exponents into floating-point numbers. */
        1.0e4,
        1.0e8,
        1.0e16,
        1.0e32,
        1.0e64,
        1.0e128,
        1.0e256
};

double atof(const char *string) {
    int sign, expSign = false;
    double fraction, dblExp, *d;
    register const char *p;
    register char c;
    int exp = 0;        /* Exponent read from "EX" field. */
    int fracExp = 0;
    int mantSize;        /* Number of digits in mantissa. */
    int decPt;
    const char *pExp;

    p = string;
    while (isspace(*p)) {
        p += 1;
    }
    if (*p == '-') {
        sign = true;
        p += 1;
    } else {
        if (*p == '+') {
            p += 1;
        }
        sign = false;
    }

    decPt = -1;
    for (mantSize = 0;; mantSize += 1) {
        c = *p;
        if (!isdigit(c)) {
            if ((c != '.') || (decPt >= 0)) {
                break;
            }
            decPt = mantSize;
        }
        p += 1;
    }

    pExp = p;
    p -= mantSize;
    if (decPt < 0) {
        decPt = mantSize;
    } else {
        mantSize -= 1;            /* One of the digits was the point. */
    }
    if (mantSize > 18) {
        fracExp = decPt - 18;
        mantSize = 18;
    } else {
        fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
        return 0.0;
    } else {
        int frac1, frac2;
        frac1 = 0;
        for (; mantSize > 9; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac1 = 10 * frac1 + (c - '0');
        }
        frac2 = 0;
        for (; mantSize > 0; mantSize -= 1) {
            c = *p;
            p += 1;
            if (c == '.') {
                c = *p;
                p += 1;
            }
            frac2 = 10 * frac2 + (c - '0');
        }
        fraction = (1.0e9 * frac1) + frac2;
    }

    p = pExp;
    if ((*p == 'E') || (*p == 'e')) {
        p += 1;
        if (*p == '-') {
            expSign = true;
            p += 1;
        } else {
            if (*p == '+') {
                p += 1;
            }
            expSign = false;
        }
        while (isdigit(*p)) {
            exp = exp * 10 + (*p - '0');
            p += 1;
        }
    }
    if (expSign)
        exp = fracExp - exp;
    else
        exp = fracExp + exp;

    if (exp < 0) {
        expSign = true;
        exp = -exp;
    } else
        expSign = false;

    if (exp > maxExponent) {
        exp = maxExponent;
    }
    dblExp = 1.0;
    for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
        if (exp & 01) {
            dblExp *= *d;
        }
    }
    if (expSign)
        fraction /= dblExp;
    else
        fraction *= dblExp;

    if (sign)
        return -fraction;

    return fraction;
}

double strtod(char *str, char **ptr) {
    char *p;
    if (ptr == (char **) 0)
        return atof(str);

    p = str;

    while (isspace(*p))
        ++p;

    if (*p == '+' || *p == '-')
        ++p;
    /* INF or INFINITY.  */
    if ((p[0] == 'i' || p[0] == 'I')
        && (p[1] == 'n' || p[1] == 'N')
        && (p[2] == 'f' || p[2] == 'F')) {
        if ((p[3] == 'i' || p[3] == 'I')
            && (p[4] == 'n' || p[4] == 'N')
            && (p[5] == 'i' || p[5] == 'I')
            && (p[6] == 't' || p[6] == 'T')
            && (p[7] == 'y' || p[7] == 'Y')) {
            *ptr = p + 8;
            return atof(str);
        } else {
            *ptr = p + 3;
            return atof(str);
        }
    }
    /* NAN or NAN(foo).  */
    if ((p[0] == 'n' || p[0] == 'N')
        && (p[1] == 'a' || p[1] == 'A')
        && (p[2] == 'n' || p[2] == 'N')) {
        p += 3;
        if (*p == '(') {
            ++p;
            while (*p != '\0' && *p != ')')
                ++p;
            if (*p == ')')
                ++p;
        }
        *ptr = p;
        return atof(str);
    }
    /* digits, with 0 or 1 periods in it.  */
    if (isdigit(*p) || *p == '.') {
        int got_dot = 0;
        while (isdigit(*p) || (!got_dot && *p == '.')) {
            if (*p == '.')
                got_dot = 1;
            ++p;
        }
        /* Exponent.  */
        if (*p == 'e' || *p == 'E') {
            int i;
            i = 1;
            if (p[i] == '+' || p[i] == '-')
                ++i;
            if (isdigit(p[i])) {
                while (isdigit(p[i]))
                    ++i;
                *ptr = p + i;
                return atof(str);
            }
        }
        *ptr = p;
        return atof(str);
    }
    /* Didn't find any digits.  Doesn't look like a number.  */
    *ptr = str;
    return 0.0;
}

long strtol(const char *nptr, char **endptr, register int base) {
    register const char *s = nptr;
    register unsigned long acc;
    register int c;
    register unsigned long cutoff;
    register int neg = 0, any, cutlim;

    do {
        c = *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+')
        c = *s++;
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    } else if ((base == 0 || base == 2) &&
               c == '0' && (*s == 'b' || *s == 'B')) {
        c = s[1];
        s += 2;
        base = 2;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    cutoff = neg ? -(unsigned long) LONG_MIN : LONG_MAX;
    cutlim = cutoff % (unsigned long) base;
    cutoff /= (unsigned long) base;
    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = neg ? LONG_MIN : LONG_MAX;
//		errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? s - 1 : nptr);
    return (acc);
}

unsigned long strtoul(const char *nptr, char **endptr, register int base) {
    register const char *s = nptr;
    register unsigned long acc;
    register int c;
    register unsigned long cutoff;
    register int neg = 0, any, cutlim;

    do {
        c = *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+')
        c = *s++;
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    } else if ((base == 0 || base == 2) &&
               c == '0' && (*s == 'b' || *s == 'B')) {
        c = s[1];
        s += 2;
        base = 2;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    cutoff = (unsigned long) ULONG_MAX / (unsigned long) base;
    cutlim = (unsigned long) ULONG_MAX % (unsigned long) base;
    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = ULONG_MAX;
//		errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? s - 1 : nptr);
    return (acc);
}