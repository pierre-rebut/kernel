//
// Created by rebut_p on 22/09/18.
//

#include <task.h>
#include <include/stdio.h>
#include "io.h"
#include "terminal.h"

#define BUFFER_SIZE 40
#define KEYBOARD_REGISTER 0x60

static int keyBuffer[BUFFER_SIZE] = {0};
static int read_ptr = 0;
static int write_ptr = 0;

char isKeyboardReady() {
    int tmp = read_ptr;
    while (tmp != write_ptr) {
        if (keyBuffer[tmp] == 28)
            return 1;
        tmp = (tmp + 1) % BUFFER_SIZE;
    }
    return 0;
}

int getkey() {
    if (read_ptr == write_ptr)
        return -1;

    int tmp = keyBuffer[read_ptr];
    read_ptr = (read_ptr + 1) % BUFFER_SIZE;

    return tmp;
}

static char keyMap[] = "&\0\"'(-\0_\0\0)=\r\tazertyuiop^$\n\0qsdfghjklm\0\0\0*wxcvbn,;:!\0\0\0 ";
// static char keyMapShift[] = "1234567890\0+\r\tAZERTYUIOP\0\0\n\0QSDFGHJKLM%\0\0\0WXCVBN?./\0";

void keyboard_handler() {
    u8 recv = inb(KEYBOARD_REGISTER);

    if ((recv >> 7))
      return;

    // check readptr circular buffer
    if ((write_ptr + 1) % BUFFER_SIZE == read_ptr)
        read_ptr = (read_ptr + 1) % BUFFER_SIZE;

    char c = keyMap[recv - 2];
    writeTerminal(c);

    keyBuffer[write_ptr] = recv;
    write_ptr = (write_ptr + 1) % BUFFER_SIZE;
}

s32 readFromKeyboard(void *entryData, void *buf, u32 size) {
    taskAddEvent(TaskEventKeyboard, 0);

    (void)entryData;

    char *str = (char*)buf;
    u32 read = 0;
    int key;

    while ((key = getkey()) != -1 && read < size) {
        char c = keyMap[key - 2];
        str[read] = c;
        read++;
    }

    return read;
}
