//
// Created by rebut_p on 20/09/18.
//

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

