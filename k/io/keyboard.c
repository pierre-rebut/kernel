//
// Created by rebut_p on 22/09/18.
//

#include <task.h>
#include <system/idt.h>
#include <system/console.h>
#include "io.h"

#define KEYBOARD_REGISTER 0x60

static void keyboard_handler(struct esp_context *ctx);

void initKeyboard()
{
    interruptRegister(33, &keyboard_handler);
}

static void keyboard_handler(struct esp_context *ctx)
{
    (void) ctx;
    u8 recv = inb(KEYBOARD_REGISTER);

    consoleKeyboardHandler(recv);
}
