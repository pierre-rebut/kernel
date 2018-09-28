//
// Created by rebut_p on 22/09/18.
//

#include "pic.h"
#include "idt.h"
#include "getkey.h"
#include "pit.h"
#include "syscall.h"
#include "task.h"

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

static void isr_exception_handler(struct esp_context *ctx) {
    if (ctx->int_no > 20)
        printf("Interrupt: %s\n", exceptionList[15]);
    else
        printf("Interrupt: %s (%d)\n", exceptionList[ctx->int_no], ctx->err_code);
}

static void isq_normal_handler(struct esp_context *ctx) {

    switch (ctx->int_no) {
        case 33:
            keyboard_handler();
            break;
        case 32:
            pit_handler();
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
        isr_exception_handler(ctx);
    else if (ctx->int_no < 48)
        isq_normal_handler(ctx);
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