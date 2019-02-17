//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#define HN_DECIMAL              0x01
#define HN_NOSPACE              0x02
#define HN_B                    0x04
#define HN_DIVISOR_1000         0x08

#define HN_GETSCALE             0x10
#define HN_AUTOSCALE            0x20
#define HN_IEC_PREFIXES         0x40

int humanize_number(char *buf, size_t len, long long quotient, const char *suffix, int scale, int flags);
char *fflagstostr(u_long flags);

#endif //KERNEL_UTILS_H
