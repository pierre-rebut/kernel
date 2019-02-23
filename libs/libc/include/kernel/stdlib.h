//
// Created by rebut_p on 23/02/19.
//

#ifndef _KERNEL_STDLIB_H
#define _KERNEL_STDLIB_H

#include "stddef.h"

int atoi(const char *str);

long atol(const char *str);

double atof(const char *string);

double strtod(char *str, char **ptr);

long strtol(const char *nptr, char **endptr, register int base);

unsigned long strtoul(const char *nptr, char **endptr, register int base);

void qsort(void *, size_t, size_t, int (*)(const void *, const void *));

#endif //_KERNEL_STDLIB_H
