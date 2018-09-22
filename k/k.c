#include "multiboot.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "getkey.h"
#include "pit.h"
#include "libvga.h"
#include "kfilesystem.h"

#include <stdio.h>

#define TEST_INTERRUPT(id) \
    printf("- Test interrupt: "#id"\n"); \
    asm volatile("int $"#id"\n")

void k_init() {
    initSerial(38400);
    printf("Serial init\n");

    initMemory();
    printf("Memory init\n");

    initInterupt();
    printf("Interrupt init\n");

    initPic();
    printf("Pic init\n");

    initPit();
    printf("Pit init\n");

    allowIrq(ISQ_KEYBOARD_VALUE);
    allowIrq(ISQ_PIT_VALUE);
}

void k_main(unsigned long magic, multiboot_info_t *info) {

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        goto error;

    k_init();

    if (info->mods_count > 0) {
        initKFileSystem((module_t*)(info->mods_addr));
    }

    TEST_INTERRUPT(0);
    TEST_INTERRUPT(3);
    TEST_INTERRUPT(30);
    TEST_INTERRUPT(15);
    TEST_INTERRUPT(20);
    TEST_INTERRUPT(16);
    TEST_INTERRUPT(6);

    (void) magic;
    (void) info;

    /*int i = 0;
    char star[4] = "|/-\\";
    char *fb = (void *) 0xb8000;*/

    listFiles();
    int fd = open("hunter", 0);
    printf("Open file: %d\n", fd);
    printf("Close file: %d\n", close(fd));

    char running = 1;

    unsigned long oldTick = 0;

    initVga();

    while (running) {
       // *fb = star[i++ % 4];

        int key = getkey();
        if (key >= 0) {
            int pressed = key >> 7;
            key &= ~(1 << 7);
            printf("Key %s: %d\n", pressed == 1 ? "release" : "pressed", key);
            if (key == 1)
                running = 0;
        }

        unsigned long tick = gettick() / 1000;
        if (tick > oldTick) {
            printf("Tick %lu\n", tick);
            oldTick = tick;
        }

        if ((gettick() / 10) % 4 == 0)
            moveBlock();
    }

    printf("Stop running\n");

    error:
    for (;;)
            asm volatile ("hlt");
}
