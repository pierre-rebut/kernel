#include <stdio.h>
#include <cpu.h>
#include <multiboot.h>
#include <include/multiboot.h>

#include "io/serial.h"
#include "sys/gdt.h"
#include "sys/idt.h"
#include "io/pic.h"
#include "io/keyboard.h"
#include "io/pit.h"
#include "io/libvga.h"
#include "io/kfilesystem.h"
#include "io/terminal.h"
#include "task.h"
#include "sys/allocator.h"
#include "sys/physical-memory.h"
#include "sys/paging.h"


#define TEST_INTERRUPT(id) \
    kSerialPrintf("- Test interrupt: "#id"\n"); \
    asm volatile("int $"#id"\n")

static int k_init(const multiboot_info_t *info) {
    initSerial(38400);
    kSerialPrintf("Init Serial\n");

    kSerialPrintf("Init Memory\n");
    initMemory();

    kSerialPrintf("Init Physical Memory\n");
    u32 memSize = initPhysicalMemory(info);
    kSerialPrintf("Total memory size: %u\n", memSize);

    kSerialPrintf("Init Paging\n");
    initPaging(memSize);

    kSerialPrintf("Init Allocator\n");
    if (initAllocator())
        return -1;

    kSerialPrintf("Init Tasking\n");
    initTasking();

    kSerialPrintf("Init Terminal\n");
    initTerminal();

    kSerialPrintf("Init Interrupt\n");
    initInterrupt();

    kSerialPrintf("Init Pic\n");
    initPic();

    kSerialPrintf("Init Pit\n");
    initPit();

    kSerialPrintf("Allow KEYBOARD & PIT interrupt\n");
    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);

    kSerialPrintf("Init KFileSystem\n");
    initKFileSystem((module_t *) info->mods_addr);

    kSerialPrintf("Start listening interruption\n");
    sti();
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

    kSerialPrintf("Write syscall result: %d\n", res);

    kfsListFiles();
    int fd = open("test", O_RDONLY);
    kSerialPrintf("Open file: %d\n", fd);

    if (fd >= 0) {
        char buf[100];
        u32 i = 0, fileLength = kfsLengthOfFile("test");
        kSerialPrintf("len of file = %d\n", fileLength);

        while (i < fileLength) {
            int tmp = read(fd, buf, 99);
            buf[tmp] = 0;

            i += tmp;
            kSerialPrintf("%s", buf);
        }
        kSerialPrintf("\nClosing file: %d\n", close(fd));
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
    taskSwitching = 0;

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || info->mods_count != 1)
        goto error;

    if (k_init(info))
        goto error;

    writeTerminalAt('0', CONS_GREEN, 0, 24);
    writeStringTerminal("Init ok\n", 8);

    kSerialPrintf("\n### Starting kernel test ###\n\n");
    k_test();
    kSerialPrintf("\n### Kernel test ok ###\n### Trying init binary [%s] ###\n\n", (char *) info->cmdline);

    writeStringTerminal("\n[F1] Clear - [F2] Start bin - [F7] - Graphic mode test - [F8] Text mode\n", 73);

    const char *av[2] = {
            (char*)info->cmdline,
            NULL
    };
    const char *env[1] = {NULL};

    createProcess((char*)info->cmdline, av, env);

    kSerialPrintf("Kernel loop start\n");
    taskSwitching = 1;

    while (1) {
        hlt();
    }

    error:
    kSerialPrintf("An error occurred\n");
    cli();
    hlt();
}
