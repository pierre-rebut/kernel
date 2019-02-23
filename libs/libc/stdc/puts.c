//
// Created by rebut_p on 23/12/18.
//

#include <string.h>
#include <stdio.h>
#include <unistd.h>

int putchar(char c)
{
    return write(1, &c, 1);
}

int puts(const char *s)
{
    return write(1, s, strlen(s));
}

int printf(const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    if (printed > 0)
        puts(printf_buf);

    return printed;
}

int dprintf(int fd, const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    if (printed > 0)
        write(fd, printf_buf, (u32) printed);

    return printed;
}

int warn(const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    if (printed > 0)
        write(2, printf_buf, (u32) printed);

    return printed;
}

int err(const char *fmt, ...)
{
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    if (printed > 0)
        write(2, printf_buf, (u32) printed);

    exit(-1);
    return 0;
}
