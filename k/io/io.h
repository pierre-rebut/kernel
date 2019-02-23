#ifndef IO_H_
#define IO_H_

#include <stddef.h>

static inline void outb(u16 port, u8 val)
{
    asm volatile("outb %0, %1\n\t"
    : /* No output */
    : "a" (val), "d" (port));
}

static inline u8 inb(u16 port)
{

    u8 res;
    asm volatile("inb %1, %0\n\t"
    : "=&a" (res)
    : "d" (port));
    return res;
}

static inline void outw(u16 port, u16 val)
{
    asm volatile("outw %0, %1\n\t"
    : /* No output */
    : "a" (val), "d" (port));
}

static inline u16 inw(u16 port)
{

    u16 res;
    asm volatile("inw %1, %0\n\t"
    : "=&a" (res)
    : "d" (port));
    return res;
}

#endif                /* !IO_H_ */
