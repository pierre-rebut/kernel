#include <stdio.h>

#include "multiboot.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"

void k_main(unsigned long magic, multiboot_info_t *info) {

    initSerial(38400);
    printf("Serial init\n");

    initMemory();
    printf("Memory init\n");

    initInterupt();
    printf("Interrupt init\n");

    initPic();
    printf("Pic init\n");

    asm volatile("int $0x0\n");

    /*asm volatile("movl $0x0, %eax\n"
            "div %ecx");*/

    allowIrq(1);
    printf("Allow irq 1\n");

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
