//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_SCANF_H
#define KERNEL_SCANF_H

#include <stdarg.h>
#include "filestream.h"

int fscanf(FILE *stream, const char *fmt, ...);
int vfscanf(FILE *stream, const char *fmt, va_list args);
int scanf(const char *fmt, ...);
int vscanf(const char *fmt, va_list args);
int sscanf(const char *buffer, const char *fmt, ...);
int vsscanf(const char *buffer, const char *fmt, va_list args);

#endif //KERNEL_SCANF_H
