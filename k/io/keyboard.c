//
// Created by rebut_p on 22/09/18.
//

#include <task.h>
#include <include/stdio.h>
#include <sys/idt.h>
#include <sys/console.h>
#include "io.h"
#include "terminal.h"

#define KEYBOARD_REGISTER 0x60

static void keyboard_handler(struct esp_context *ctx);

void initKeyboard() {
    interruptRegister(33, &keyboard_handler);
}

static void keyboard_handler(struct esp_context *ctx) {
    (void)ctx;
    u8 recv = inb(KEYBOARD_REGISTER);

    if ((recv >> 7))
      return;

    consoleKeyboardHandler(recv);
}
