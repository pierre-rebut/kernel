//
// Created by rebut_p on 29/09/18.
//

#include <stdio.h>
#include "mouse.h"
#include "io.h"

#define MOUSE_REGISTER 0x60

struct Mouse {
    int x, y, buttons;
};

static struct Mouse mouse = {0};

void initMouse() {
    outb(MOUSE_REGISTER, 0xF4);
    u8 tmp = inb(MOUSE_REGISTER);
    printf("mouse: %d\n", tmp);
}

int getmouse(int *x, int *y, int *buttons) {
    *x = mouse.x;
    *y = mouse.y;
    *buttons = mouse.buttons;

    mouse.buttons = 0;
    return 0;
}

void mouse_handler() {
    u8 tmp = inb(MOUSE_REGISTER);
    printf("mouse1: %d\n", tmp);
    tmp = inb(MOUSE_REGISTER);
    printf("mouse2: %d\n", tmp);
    tmp = inb(MOUSE_REGISTER);
    printf("mouse3: %d\n", tmp);
}