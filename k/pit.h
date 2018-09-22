//
// Created by rebut_p on 22/09/18.
//

#ifndef KERNEL_EPITA_PIT_H
#define KERNEL_EPITA_PIT_H

#define ISQ_PIT_VALUE 0

void initPit();

unsigned long gettick();
void recvPit();

#endif //KERNEL_EPITA_PIT_H
