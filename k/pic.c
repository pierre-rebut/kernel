//
// Created by rebut_p on 22/09/18.
//

#include <stdio.h>
#include "pic.h"
#include "io.h"

#define PIC_MASK_ALL 0xFF

#define PIC_MASTER_A 0x20
#define PIC_SLAVE_A 0xA0
#define PIC_MASTER_B 0x21
#define PIC_SLAVE_B 0xA1

void initPic() {
    outb(PIC_MASTER_A, 0x11);
    outb(PIC_SLAVE_A, 0x11);

    outb(PIC_MASTER_B, 32);
    outb(PIC_SLAVE_B, 40);

    outb(PIC_MASTER_B, 0x04);
    outb(PIC_SLAVE_B, 0x02);

    outb(PIC_MASTER_B, 0x01);
    outb(PIC_SLAVE_B, 0x01);

    // mask all interrupt
    outb(PIC_MASTER_B, PIC_MASK_ALL);
    outb(PIC_SLAVE_B, PIC_MASK_ALL);

    asm volatile("sti");
}

void allowIrq(int irq) {

    if (irq < 8) {
        u8 tmp = inb(PIC_MASTER_B);
        tmp &= ~(1 << irq);
        outb(PIC_MASTER_B, tmp);
    } else {
        u8 tmp = inb(PIC_SLAVE_B);
        tmp &= ~(1 << (irq - 8));
        outb(PIC_SLAVE_B, tmp);
    }

    printf("Allow irq %d\n", irq);
}

void pic_eoi_master(int irq) {
    (void)irq;
    outb(PIC_MASTER_A, 0x20);
}

void pic_eoi_slave(int irq) {
    (void)irq;
    outb(PIC_SLAVE_A, 0x20);
}