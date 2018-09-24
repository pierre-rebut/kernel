#include "multiboot.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "getkey.h"
#include "pit.h"
#include "libvga.h"
#include "kfilesystem.h"
#include "terminal.h"

#include <stdio.h>

#define TEST_INTERRUPT(id) \
    printf("- Test interrupt: "#id"\n"); \
    asm volatile("int $"#id"\n")

static void k_init() {
    initSerial(38400);
    printf("Serial init\n");

    initMemory();
    printf("Memory init\n");

    initTerminal();
    printf("Terminal init\n");

    initInterrupt();
    printf("Interrupt init\n");

    initPic();
    printf("Pic init\n");

    initPit();
    printf("Pit init\n");

    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);
}

static void k_test() {
    TEST_INTERRUPT(0);
    TEST_INTERRUPT(3);
    TEST_INTERRUPT(30);
    TEST_INTERRUPT(15);
    TEST_INTERRUPT(20);
    TEST_INTERRUPT(16);
    TEST_INTERRUPT(6);

    listFiles();
    int fd = open("hunter", 0);
    printf("Open file: %d\n", fd);
    printf("Close file: %d\n", close(fd));
}

static char asciiTab[87][3] = {
        [2] = {'&', '1'},
        [3] = {'\0', '2', '~'},
        [4] = {'"', '3', '#'},
        [5] = {'\'', '4', '{'},
        [6] = {'(', '5', '['},
        [7] = {'-', '6', '|'},
        [8] = {'\0', '7', '`'},
        [9] = {'_', '8', '\\'},
        [10] = {'\0', '9', '^'},
        [11] = {'\0', '0', '@'},
        [12] = {')', '\0', ']'},
        [13] = {'=', '+', '}'},
        [14] = {'\r'},
        [15] = {'\t'},
        [16] = {'a', 'A'},
        [17] = {'z', 'Z'},
        [18] = {'e', 'E'},
        [19] = {'r', 'R'},
        [20] = {'t', 'T'},
        [21] = {'y', 'Y'},
        [22] = {'u', 'U'},
        [23] = {'i', 'I'},
        [24] = {'o', 'O'},
        [25] = {'p', 'P'},
        [26] = {'^'},
        [27] = {'$'},
        [28] = {'\n'},
        [30] = {'q', 'Q'},
        [31] = {'s', 'S'},
        [32] = {'d', 'D'},
        [33] = {'f', 'F'},
        [34] = {'g', 'G'},
        [35] = {'h', 'H'},
        [36] = {'j', 'J'},
        [37] = {'k', 'K'},
        [38] = {'l', 'L'},
        [39] = {'m', 'M'},
        [40] = {'\0', '%'},
        [43] = {'*'},
        [86] = {'<', '>'},
        [44] = {'w', 'W'},
        [45] = {'x', 'X'},
        [46] = {'c', 'C'},
        [47] = {'v', 'V'},
        [48] = {'b', 'B'},
        [49] = {'n', 'N'},
        [50] = {',', '?'},
        [51] = {';', '.'},
        [52] = {':', '/'},
        [53] = {'!'},
        [57] = {' '}
};

static char running = 1;
static int keyMode[3] = {0};

static void keyHandler(int key) {
    int release = key >> 7;
    key &= ~(1 << 7);

    if (key == 42 || key == 54) {
        keyMode[0] = (release ? 0 : 1);
        return;
    }

    if (key == 29 || key == 96) {
        keyMode[1] = (release ? 0 : 1);
        return;
    }

    if (release == 1)
        return;

    switch (key) {
        case KEY_ESC:
            running = 0;
            break;
        case KEY_F1:
            clearTerminal();
            break;
        case KEY_MAJLOCK:
            keyMode[2] = (keyMode[2] ? 0 : 1);
            break;
        default:
            if (key < 87) {
                int mode = 0;
                if (keyMode[0] == 1)
                    mode = 1;
                else if (keyMode[1] == 1)
                    mode = 2;
                else if (keyMode[2] == 1)
                    mode = 1;

                char c = asciiTab[key][mode];
                if (c != '\0')
                    writeTerminal(c);
                else
                    writeStringTerminal("^@", 2);
            }
    }
}

void my_putnbr(unsigned long n, size_t pos) {
    if (n > 9) {
        my_putnbr(n / 10, pos++);
        my_putnbr(n % 10, pos++);
    } else
        writeTerminalAt(n + '0', CONS_GREEN, pos, 24);
}

void k_main(unsigned long magic, multiboot_info_t *info) {

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        goto error;

    k_init();

    if (info->mods_count > 0) {
        initKFileSystem((module_t *) (info->mods_addr));
    }

    k_test();

    unsigned long oldTick = 0;
    writeTerminalAt('0', CONS_GREEN, 0, 24);

    // initVga();
    //libvga_switch_mode3h();

    writeStringTerminal("Init ok\n", 8);

    while (running) {
        int key = getkey();
        if (key >= 0) {
            keyHandler(key);
        }

        unsigned long tick = gettick() / 1000;
        if (tick > oldTick) {
            my_putnbr(tick, 0);
            oldTick = tick;
        }

        /*if ((gettick() / 10) % 4 == 0)
            moveBlock();*/
    }

    printf("Stop running\n");

    error:
    for (;;)
            asm volatile ("hlt");
}

int write(const char *s, size_t i) {
    return writeSerial((const void *) s, i);
}