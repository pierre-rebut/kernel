//
// Created by rebut_p on 16/02/19.
//

#ifndef _STDIO_H_
#define _STDIO_H_

#include <kernel/stdio.h>
#include "filestream.h"

int putchar(char c);

int puts(const char *s);

int printf(const char *format, ...);

int dprintf(int fd, const char *fmt, ...);

int fscanf(FILE *stream, const char *fmt, ...);

int vfscanf(FILE *stream, const char *fmt, va_list args);

int scanf(const char *fmt, ...);

int vscanf(const char *fmt, va_list args);

int sscanf(const char *buffer, const char *fmt, ...);

int vsscanf(const char *buffer, const char *fmt, va_list args);

#endif                /* !_STDIO_H_ */
