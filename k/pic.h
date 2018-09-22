//
// Created by rebut_p on 22/09/18.
//

#ifndef KERNEL_EPITA_PIC_H
#define KERNEL_EPITA_PIC_H

void initPic();
void allowIrq(int irq);
void pic_eoi_master(int irq);
void pic_eoi_slave(int irq);

#endif //KERNEL_EPITA_PIC_H
