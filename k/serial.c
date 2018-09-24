#include "io.h"
#include "serial.h"

#define COM1 0x3F8

void initSerial(int bps) {
    int div = 115200 / bps;
    outb(COM1 + 3, 0x80);
    outb(COM1, (u8)(div & 0xFF));
    outb(COM1 + 1, (u8)(div >> 8));

    outb(COM1 + 3, 0x3);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 1, 0x1);
}

int writeSerial(const void *str, size_t length) {
    int res = 0;
    for (size_t i = 0; i < length; i++) {
        outb(COM1, ((u8 *) str)[i]);
        res ++;
    }
    return res;
}
