//
// Created by rebut_p on 22/09/18.
//

#ifndef KERNEL_EPITA_KEYBOARD_H
#define KERNEL_EPITA_KEYBOARD_H

#define ISQ_KEYBOARD_VALUE 1

char isKeyboardReady();
int getkey();
void keyboard_handler();

s32 readFromKeyboard(void *, void *buf, u32 size);

#endif // KERNEL_EPITA_KEYBOARD_H
