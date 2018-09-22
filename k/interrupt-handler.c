//
// Created by rebut_p on 22/09/18.
//

#include "pic.h"
#include "io.h"
#include "idt.h"

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

static void isr_exception_handler(struct idt_context *ctx) {
    if (ctx->int_no > 20)
        printf("Interrupt: %s\n", exceptionList[15]);
    else
        printf("Interrupt: %s\n", exceptionList[ctx->int_no]);
}

static void isq_normal_handler(struct idt_context *ctx) {
    printf("Interrupt ISQ handle: %d\n", ctx->int_no);

    if (ctx->int_no == 33) {
        u8 tmp = inb(0x60);
        printf("scancode: %d\n", tmp);
    }

    if (ctx->int_no >= 40)
        pic_eoi_slave();
    pic_eoi_master();
}

static void isr_normal_handler(struct idt_context *ctx) {
    printf("Interrupt ISR handle: %d\n", ctx->int_no);
}

void interrupt_handler(struct idt_context *ctx) {
    if (ctx->int_no < 32)
        isr_exception_handler(ctx);
    else if (ctx->int_no < 48)
        isq_normal_handler(ctx);
    else
        isr_normal_handler(ctx);
}