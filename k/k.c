#include "multiboot.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "pit.h"
#include "libvga.h"
#include "kfilesystem.h"
#include "terminal.h"
#include "binary.h"
#include "task.h"
#include "allocator.h"
#include "physical-memory.h"
#include "paging.h"
#include "cpu.h"

#include <stdio.h>

#define TEST_INTERRUPT(id) \
    printf("- Test interrupt: "#id"\n"); \
    asm volatile("int $"#id"\n")

static int k_init(const multiboot_info_t *info) {
    initSerial(38400);
    printf("Init Serial\n");

    printf("Init Memory\n");
    initMemory();

    printf("Init Temporary allocator\n");
    initTemporaryAllocator(info);

    printf("Init Physical Memory\n");
    u32 memSize = initPhysicalMemory(info);
    printf("Total memory size: %u\n", memSize);

    printf("Init Paging\n");
    initPaging(memSize);

    printf("Init Allocator\n");
    if (initAllocator())
        return -1;

    printf("Init Terminal\n");
    initTerminal();

    printf("Init Interrupt\n");
    initInterrupt();

    printf("Init Pic\n");
    initPic();

    printf("Init Pit\n");
    initPit();

    printf("Allow KEYBOARD & PIT interrupt\n");
    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);

    return 0;
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
    asm volatile ("int $0x80" : "=a"(res) : "a"(0), "b"((u32) str), "c"(17));

    printf("Write syscall result: %d\n", res);

    listFiles();
    int fd = open("test", O_RDONLY);
    printf("Open file: %d\n", fd);

    if (fd >= 0) {
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
}

static char keyMap[] = "&\0\"'(-\0_\0\0)=\r\tazertyuiop^$\n\0qsdfghjklm\0\0\0*wxcvbn,;:!";
static char keyMapShift[] = "1234567890\0+\r\tAZERTYUIOP\0\0\n\0QSDFGHJKLM%\0\0\0WXCVBN?./\0";
// static char keyMapCtrl[] = "\0~#{[|`\\^@]}\r\t";

static char running = 1;
static char isUpper = 0;

static void keyHandler(int key) {
    switch (key) {
        case KEY_ESC:
            running = 0;
            break;
        case KEY_F1:
            clearTerminal();
            break;
        case KEY_F2:
            printf("### Trying executing binary ###\n");
            launchTask();
            break;
        case KEY_F7:
            switchVgaMode(VIDEO_GRAPHIC);
            break;
        case KEY_F8:
            switchVgaMode(VIDEO_TEXT);
            break;
        case KEY_MAJLOCK:
            isUpper = (char) (isUpper ? 0 : 1);
            break;
        case 29:
        case 42:
            break;
        default:
            if (getVideoMode() != VIDEO_TEXT)
                return;

            if (key == 57)
                writeTerminal(' ');
            else {
                char c = 0;

                if (key == 86)
                    c = (char) (isUpper ? '>' : '<');
                else if (key < 53 && key >= 2) {
                    if (isUpper)
                        c = keyMapShift[key - 2];
                    else
                        c = keyMap[key - 2];
                }

                if (!c)
                    writeStringTerminal("^@", 2);
                else
                    writeTerminal(c);
            }
    }
}

void my_putnbr(unsigned long n, u32 pos) {
    if (n > 9) {
        my_putnbr(n / 10, pos++);
        my_putnbr(n % 10, pos++);
    } else
        writeTerminalAt(n + '0', CONS_GREEN, pos, 24);
}

void k_main(unsigned long magic, multiboot_info_t *info) {

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || info->mods_count != 1)
        goto error;

    if (k_init(info)) {
        cli();
        hlt();
    }

    unsigned long oldTick = 0;
    writeTerminalAt('0', CONS_GREEN, 0, 24);
    writeStringTerminal("Init ok\n", 8);

    initKFileSystem((module_t *) info->mods_addr);

    printf("\n### Starting kernel test ###\n\n");
    k_test();
    printf("\n### Kernel test ok ###\n### Trying init binary [%s] ###\n\n", (char *) info->cmdline);
    createTask((const char*)info->cmdline);

    writeStringTerminal("\n[F1] Clear - [F2] Start bin - [F7] - Graphic mode test - [F8] Text mode\n", 73);

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

int write(const char *s, u32 i) {
    return writeSerial((const void *) s, i);
}
