//
// Created by rebut_p on 23/09/18.
//

#include "terminal.h"
#include "serial.h"
#include "getkey.h"
#include "pit.h"
#include "kfilesystem.h"
#include "syscall.h"

#include <k/kstd.h>

static void sys_write(struct idt_context *ctx);
static void sys_writeSerial(struct idt_context *ctx);
static void sys_sbrk(struct idt_context *ctx);
static void sys_getkey(struct idt_context *ctx);
static void sys_gettick(struct idt_context *ctx);
static void sys_open(struct idt_context *ctx);
static void sys_read(struct idt_context *ctx);
static void sys_seek(struct idt_context *ctx);
static void sys_close(struct idt_context *ctx);


typedef void (*syscall_t)(struct idt_context *);

static syscall_t syscall[] = {
        sys_write, // SYSCALL_WRITE
        sys_writeSerial, // SYSCALL_WRITESERIAL
        sys_sbrk, // SYSCALL_SBR
        sys_getkey, // SYSCALL_GETKEY
        sys_gettick, // SYSCALL_GETTICK
        sys_open, // SYSCALL_OPEN
        sys_read, // SYSCALL_READ
        sys_seek, // SYSCALL_SEEK
        sys_close, // SYSCALL_CLOSE
        NULL, // SYSCALL_SETVIDEO
        NULL, // SYSCALL_SWAP_FRONTBUFFER
        NULL, // SYSCALL_PLAYSOUND
        NULL, // SYSCALL_GETMOUSE
        NULL, // SYSCALL_GETKEYMODE
};

void syscall_handler(struct idt_context *ctx) {
    if (ctx->eax >= NR_SYSCALL)
        return;

    syscall_t fct = syscall[ctx->eax];
    if (fct == NULL)
        return;

    fct(ctx);
}

/*** SYSCALL FCT ***/

static void sys_write(struct idt_context *ctx) {
    writeStringTerminal((const char *)ctx->ebx, ctx->ecx);
    ctx->eax = ctx->ecx;
}

static void sys_writeSerial(struct idt_context *ctx) {
    writeSerial((const void *)ctx->ebx, ctx->ecx);
    ctx->eax = ctx->ecx;
}

static void sys_sbrk(struct idt_context *ctx) {
    (void) ctx;
    // todo
}

static void sys_getkey(struct idt_context *ctx) {
    int tmp = getkey();
    ctx->eax = (u32) tmp;
}

static void sys_gettick(struct idt_context *ctx) {
    unsigned long tmp = gettick();
    ctx->eax = (u32) tmp;
}

static void sys_open(struct idt_context *ctx) {
    int tmp = open((const char *)ctx->ebx, ctx->ecx);
    ctx->eax = (u32)tmp;
}

static void sys_read(struct idt_context *ctx) {
    ssize_t tmp = read(ctx->ebx, (void *)ctx->ecx, ctx->edx);
    ctx->eax = (u32) tmp;
}

static void sys_seek(struct idt_context *ctx) {
    off_t tmp = seek(ctx->ebx, (off_t)ctx->ecx, ctx->edx);
    ctx->eax = (u32) tmp;
}

static void sys_close(struct idt_context *ctx) {
    int tmp = close(ctx->ebx);
    ctx->eax = (u32) tmp;
}