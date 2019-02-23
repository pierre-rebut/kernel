//
// Created by rebut_p on 22/02/19.
//

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <kernel/stdlib.h>

void *malloc(size_t len);

void *realloc(void *ptr, size_t len);

void *calloc(size_t len, size_t);

void free(const void *ptr);

void show_alloc_mem();

long long strtonum(const char *numstr, long long minval, long long maxval, const char **errstrp);

long long int strtoll(const char *nptr, char **endptr, int base);

#endif /* !_STDLIB_H_ */
