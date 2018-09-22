//
// Created by rebut_p on 22/09/18.
//

#include <stdio.h>
#include "pit.h"
#include "io.h"

#define PIT_CONTROL_REGISTER 0x43
#define PIT_COUNTER0 0x40

static unsigned long nbTick = 0;

void initPit() {
    outb(PIT_CONTROL_REGISTER, 2);
    outb(PIT_COUNTER0, 1193182 / 100);
}

unsigned long gettick() {
    return nbTick;
}

void recvPit() {
    nbTick++;
    printf("Tick: %lu\n", nbTick);
}