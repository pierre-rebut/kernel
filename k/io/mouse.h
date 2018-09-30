//
// Created by rebut_p on 29/09/18.
//

#ifndef KERNEL_EPITA_MOUSE_H
#define KERNEL_EPITA_MOUSE_H

#define ISQ_MOUSE_VALUE 12

void initMouse();

int getmouse(int *x, int *y, int *buttons);
void mouse_handler();

#endif //KERNEL_EPITA_MOUSE_H
