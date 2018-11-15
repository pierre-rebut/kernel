//
// Created by rebut_p on 22/09/18.
//

#include "io.h"

#define BUFFER_SIZE 40
#define KEYBOARD_REGISTER 0x60

static int keyBuffer[BUFFER_SIZE] = {0};
static int read_ptr = 0;
static int write_ptr = 0;

int getkey() {
    if (read_ptr == write_ptr)
        return -1;

    int tmp = keyBuffer[read_ptr];
    read_ptr = (read_ptr + 1) % BUFFER_SIZE;

    return tmp;
}

void keyboard_handler() {
    u8 recv = inb(KEYBOARD_REGISTER);

    if ((recv >> 7))
      return;

    // check readptr circular buffer
    if ((write_ptr + 1) % BUFFER_SIZE == read_ptr)
        read_ptr = (read_ptr + 1) % BUFFER_SIZE;

    keyBuffer[write_ptr] = recv;
    write_ptr = (write_ptr + 1) % BUFFER_SIZE;
}
