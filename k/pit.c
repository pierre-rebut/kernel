//
// Created by rebut_p on 22/09/18.
//

#include "pit.h"
#include "io.h"

#define PIT_CONTROL_REGISTER 0x43
#define PIT_COUNTER0 0x40

#define PIT_MODE2 (0x2 << 1)
#define PIT_RW_POLICY (0x3 << 4)

static unsigned long nbTick = 0;

void initPit() {
    outb(PIT_CONTROL_REGISTER, PIT_RW_POLICY | PIT_MODE2 | 0x1);

    outb(PIT_COUNTER0, (u8)((1193182 / 100) >> 0));
    outb(PIT_COUNTER0, (u8)((1193182 / 100) >> 8));
}

unsigned long gettick() {
    return nbTick;
}

void pit_handler() {
    nbTick += 10;
}