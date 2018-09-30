//
// Created by rebut_p on 22/09/18.
//

#include "io/pic.h"
#include "sys/idt.h"
#include "io/keyboard.h"
#include "io/pit.h"
#include "sys/syscall.h"
#include "task.h"
#include "io/mouse.h"

#include <stdio.h>

static char *exceptionList[] = {
        "Division by zero",
        "Debug",
        "NMI",
        "Breakpoint",
        "Overflow",
        "Bound Range Exceeded",
        "Invalid Opcode",
        "Device Not Available",
        "Double Fault",
        "Coprocessor",
        "Invalid TSS",
        "Segment not present",
        "Stack Segment Fault",
        "General Protection",
        "Page Fault",
        "Intel Reserved",
        "x87 FPU",
        "Alignment Check",
        "Machine Check",
        "SMD Floating Point Exception",
        "Virtualization Exception"
};

static void exception_handler(struct esp_context *ctx) {
    if (ctx->int_no > 20)
        printf("Interrupt: %s\n", exceptionList[15]);
    else
        printf("Interrupt: %s (%d)\n", exceptionList[ctx->int_no], ctx->eip);
}

static void isq_handler(struct esp_context *ctx) {

    switch (ctx->int_no) {
        case 32:
            pit_handler();
            break;
        case 33:
            keyboard_handler();
            break;
        case 44:
            printf("toto\n");
            mouse_handler();
            break;
        default:
            printf("Interrupt ISQ handle: %d\n", ctx->int_no);
    }

    if (ctx->int_no >= 40)
        pic_eoi_slave(ctx->int_no);
    pic_eoi_master(ctx->int_no);
}

u32 interrupt_handler(u32 esp) {
    struct esp_context *ctx = (struct esp_context *) esp;

    if (ctx->int_no < 32)
        exception_handler(ctx);
    else if (ctx->int_no < 48)
        isq_handler(ctx);
    else {
        switch (ctx->int_no) {
            case 0x80:
                syscall_handler(ctx);
                break;
            case 50:
                esp = task_switch(esp);
                break;
            default:
                printf("Interrupt ISR handle: %d\n", ctx->int_no);
        }
    }
    return esp;
}