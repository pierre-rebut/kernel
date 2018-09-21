//
// Created by rebut_p on 22/09/18.
//

#ifndef KERNEL_EPITA_PIC_H
#define KERNEL_EPITA_PIC_H

void initPic();
void allowIrq(int irq);
void pic_eoi_master();

#define PIC_MASTER_A 0x20
#define PIC_SLAVE_A 0xA0
#define PIC_MASTER_B 0x21
#define PIC_SLAVE_B 0xA1

#endif //KERNEL_EPITA_PIC_H
