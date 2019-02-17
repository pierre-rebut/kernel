//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_GETNBR_H
#define KERNEL_GETNBR_H

long long strtonum(const char *numstr, long long minval, long long maxval, const char **errstrp);
long long int strtoll(const char *nptr, char **endptr, int base);

#endif //KERNEL_GETNBR_H
