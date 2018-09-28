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
#include "binary.h"

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

    u32 res;
    char *str = "Syscall write ok\n";
    asm volatile ("int $0x80" : "=a"(res) : "a"(0), "b"((u32)str), "c"(17));

    printf("Write syscall result: %d\n", res);

    listFiles();
    int fd = open("test", O_RDONLY);
    printf("Open file: %d\n", fd);

    char buf[100];

    u32 i = 0, fileLength = length("test");
    while (i < fileLength) {

        int tmp = read(fd, buf, 99);
        buf[tmp] = 0;

        i += tmp;

        printf("%s", buf);
    }
    printf("\nClosing file: %d\n", close(fd));
}

static char keyMap[] = "&\0\"'(-\0_\0\0)=\r\tazertyuiop^$\n\0qsdfghjklm\0*<wxcvbn,;:!";
static char keyMapShift[] = "1234567890°+\r\tAZERTYUIOP\0\0\n\0QSDFGHJKLM%\0>WXCVBN?./\0";
static char keyMapCtrl[] = "\0~#{[|`\\^@]}\r\t";

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
        case KEY_F2:
            printf("### Trying executing binary ###\n");
            asm volatile("int $50");
            break;
        case KEY_F7:
            switchVgaMode(VIDEO_TEXT);
            break;
        case KEY_F8:
            switchVgaMode(VIDEO_TEXT);
            break;
        case KEY_MAJLOCK:
            keyMode[2] = (keyMode[2] ? 0 : 1);
            break;
        default:
            if (getVideoMode() != VIDEO_TEXT)
                return;

            if (key < 87 && key >= 2) {
                if (key == 57) {
                    writeTerminal(' ');
                } else {
                    char c;

                    if (keyMode[0] == 1)
                        c = keyMapShift[key - 2];
                    else if (keyMode[1] == 1) {
                        if (key > 16)
                            c = '\0';
                        else
                            c = keyMapCtrl[key];
                    } else if (keyMode[2] == 1)
                        c = keyMapShift[key - 2];
                    else
                        c = keyMap[key - 2];

                    if (c == '\0')
                        writeStringTerminal("^@", 2);
                    else
                        writeTerminal(c);
                }
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

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || info->mods_count != 1)
        goto error;

    k_init();
    unsigned long oldTick = 0;
    writeTerminalAt('0', CONS_GREEN, 0, 24);
    writeStringTerminal("Init ok\n", 8);

    initKFileSystem((module_t *) info->mods_addr);

    printf("\n### Starting kernel test ###\n\n");
    k_test();
    printf("\n### Kernel test ok ###\n### Trying init binary [%s] ###\n\n", (char*)info->cmdline);
    loadBinary((module_t *) info->mods_addr, info->cmdline);


    while (running) {
        int key = getkey();
        if (key >= 0) {
            keyHandler(key);
        }

        int videoMode = getVideoMode();

        unsigned long tick = gettick() / 1000;
        if (videoMode == VIDEO_TEXT && tick > oldTick) {
            my_putnbr(tick, 0);
            oldTick = tick;
        }

        if (videoMode == VIDEO_GRAPHIC && (gettick() / 10) % 4 == 0)
            moveBlock();
    }

    printf("Stop running\n");

    error:
    for (;;)
            asm volatile ("hlt");
}

int write(const char *s, size_t i) {
    return writeSerial((const void *) s, i);
}