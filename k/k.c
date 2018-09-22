#include "multiboot.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"

#include <stdio.h>

#define TEST_INTERRUPT(id) \
    printf("- Test interrupt: "#id"\n"); \
    asm volatile("int $"#id"\n")

void k_main(unsigned long magic, multiboot_info_t *info) {

    initSerial(38400);
    printf("Serial init\n");

    initMemory();
    printf("Memory init\n");

    initInterupt();
    printf("Interrupt init\n");

    initPic();
    printf("Pic init\n");

    TEST_INTERRUPT(0);
    TEST_INTERRUPT(3);
    TEST_INTERRUPT(30);
    TEST_INTERRUPT(15);
    TEST_INTERRUPT(20);
    TEST_INTERRUPT(16);
    TEST_INTERRUPT(6);

    allowIrq(1);
    // allowIrq(0);

    (void) magic;
    (void) info;

    char star[4] = "|/-\\";
    char *fb = (void *) 0xb8000;

    for (unsigned i = 0;;) {
        *fb = star[i++ % 4];
    }

    for (;;)
            asm volatile ("hlt");
}
