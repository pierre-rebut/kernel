//
// Created by rebut_p on 22/09/18.
//

#include "io/pic.h"
#include "sys/idt.h"
#include "io/keyboard.h"
#include "io/pit.h"
#include "sys/syscall.h"
#include "task.h"
#include "allocator.h"

#include <stdio.h>
#include <sheduler.h>
#include <include/cpu.h>

struct InterruptList {
    u32 id;

    void (*fct)(struct esp_context *);

    struct InterruptList *next;
};

static struct InterruptList *intList = NULL;

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

static void quitTask(void) {
    if (currentTask == NULL)
        return;

    if (currentTask->pid == 0) {
        kSerialPrintf("<KERNEL PANIC> !!!!!!!\n\n");
        cli();
        hlt();
    }

    kSerialPrintf("<TASK ERROR>: %u\n\n", currentTask->pid);
    taskExit();
}

static void isr_exception_handler(struct esp_context *r) {
    if (r->int_no > 20)
        kSerialPrintf("[INT] %s\n", exceptionList[15]);
    else
        kSerialPrintf("[INT] %s\n", exceptionList[r->int_no]);

    kSerialPrintf("[INT] err code: %u eip: %X\n", r->err_code, r->eip);
    kSerialPrintf("[INT] edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax,
                  r->ebx, r->ecx, r->edx);
    kSerialPrintf("[INT] cs: %x  ds: %x  es: %x  Fs: %x  gs: %x  ss: %x\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
    kSerialPrintf("[INT] eflags: %X  useresp: %X\n", r->eflags, r->useresp);

    quitTask();
}

int interruptRegister(u32 int_no, void (*fct)(struct esp_context *)) {
    struct InterruptList *newElem = kmalloc(sizeof(struct InterruptList), 0, "InterruptList");
    if (newElem == NULL)
        return -1;

    newElem->id = int_no;
    newElem->fct = fct;
    newElem->next = intList;
    intList = newElem;
    return 0;
}

static void executeInterruptFromLis(struct esp_context *ctx) {
    struct InterruptList *elem = intList;
    while (elem != NULL) {
        if (elem->id == ctx->int_no) {
            elem->fct(ctx);
            return;
        }
        elem = elem->next;
    }
    kSerialPrintf("Interrupt not found: %d\n", ctx->int_no);
}

u32 interrupt_handler(u32 esp) {
    struct esp_context *ctx = (struct esp_context *) esp;

    if (ctx->int_no < 48) {
        if (ctx->int_no >= 40)
            pic_eoi_slave(ctx->int_no);
        pic_eoi_master(ctx->int_no);
    }

    if ((ctx->int_no == 32 && taskSwitching) || ctx->int_no == 126) {
        esp = schedulerSwitchTask(esp);
    }

    if (ctx->int_no < 32)
        isr_exception_handler(ctx);
    else if (ctx->int_no != 126) {
        executeInterruptFromLis(ctx);
    }

    return esp;
}