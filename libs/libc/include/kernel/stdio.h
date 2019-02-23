//
// Created by rebut_p on 23/02/19.
//

#ifndef _KERNEL_STDIO_H
#define _KERNEL_STDIO_H

#include "stdarg.h"
#include "stddef.h"

#define EOF (-1)
#define BUFSIZ 8192

int sprintf(char *buf, const char *format, ...);

int snprintf(char *buf, u32 size, const char *format, ...);

int vsprintf(char *buf, const char *format, va_list args);

int vsnprintf(char *buf, u32 size, const char *fmt, va_list args);

#endif //_KERNEL_STDIO_H
