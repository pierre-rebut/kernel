/* -*- linux-c -*- ------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *
 *   This file is part of the Linux kernel, and is made available under
 *   the terms of the GNU General Public License version 2.
 *
 * ----------------------------------------------------------------------- */

/*
 * Oh, it's a waste of space, but oh-so-yummy for debugging.  This
 * version of printf() does not include 64-bit support.  "Live with
 * it."
 *
 */

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <system/console.h>
#include <io/serial.h>

int kputs(const char *s)
{
    return consoleForceWrite(s, strlen(s));
}

int kprintf(const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    if (printed > 0)
        consoleForceWrite(printf_buf, printed);

    return printed;
}

int klog(const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    if (printed > 0)
        writeSerial(printf_buf, (u32) printed);

    return printed;
}

