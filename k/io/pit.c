//
// Created by rebut_p on 22/09/18.
//

#include <system/idt.h>
#include "pit.h"
#include "io.h"

#define PIT_CONTROL_REGISTER 0x43
#define PIT_COUNTER0 0x40

#define PIT_MODE2 (0x2 << 1)
#define PIT_RW_POLICY (0x3 << 4)

static unsigned long nbTick = 0;

static void pit_handler(struct esp_context *);

void initPit()
{
    interruptRegister(32, &pit_handler);

    outb(PIT_CONTROL_REGISTER, PIT_RW_POLICY | PIT_MODE2 | 0x1);

    outb(PIT_COUNTER0, (u8) ((1193182 / 100) >> 0));
    outb(PIT_COUNTER0, (u8) ((1193182 / 100) >> 8));
}

unsigned long gettick()
{
    return nbTick;
}

static void pit_handler(struct esp_context *ctx)
{
    (void) ctx;
    nbTick += 10;
}