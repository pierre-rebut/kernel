//
// Created by rebut_p on 23/09/18.
//

#include "io/serial.h"
#include "io/keyboard.h"
#include "io/pit.h"
#include "io/kfilesystem.h"
#include "io/terminal.h"
#include "syscall.h"
#include "io/libvga.h"
#include "task.h"

#include <stdio.h>

static void sys_write(struct esp_context *ctx);
static void sys_sbrk(struct esp_context *ctx);
static void sys_getkey(struct esp_context *ctx);
static void sys_gettick(struct esp_context *ctx);
static void sys_open(struct esp_context *ctx);
static void sys_read(struct esp_context *ctx);
static void sys_seek(struct esp_context *ctx);
static void sys_close(struct esp_context *ctx);
static void sys_setvideo(struct esp_context *ctx);
static void sys_setVgaFrameBuffer(struct esp_context *ctx);
static void sys_getMouse(struct esp_context *ctx);
static void sys_playsound(struct esp_context *ctx);
static void sys_getkeymode(struct esp_context *ctx);
static void sys_sleep(struct esp_context *ctx);
static void sys_waitPid(struct esp_context *ctx);
static void sys_exit(struct esp_context *ctx);
static void sys_kill(struct esp_context *ctx);
static void sys_getPid(struct esp_context *ctx);

typedef void (*syscall_t)(struct esp_context *);

static syscall_t syscall[] = {
        sys_exit,
        sys_sbrk,
        sys_getkey,
        sys_gettick,
        sys_open,
        sys_read,
        sys_write,
        sys_seek,
        sys_close,
        sys_setvideo,
        sys_setVgaFrameBuffer,
        sys_playsound,
        sys_getMouse,
        sys_getkeymode,
        sys_sleep,
        sys_waitPid,
        sys_kill,
        sys_getPid
};

void syscall_handler(struct esp_context *ctx) {
    if (ctx->eax >= NR_SYSCALL)
        return;

    syscall_t fct = syscall[ctx->eax];
    if (fct == NULL) {
        kSerialPrintf("unhandled syscall %d\n", ctx->eax);
        ctx->eax = 0;
        return;
    }

    fct(ctx);
}

/*** SYSCALL FCT ***/

static void sys_exit(struct esp_context *ctx) {
    ctx->eax = (u32) taskExit();
}

static void sys_sbrk(struct esp_context *ctx) {
    ctx->eax = taskSetHeapInc((s32)ctx->ebx);
}

static void sys_getkey(struct esp_context *ctx) {
    int tmp = getkey();
    ctx->eax = (u32) tmp;
}

static void sys_gettick(struct esp_context *ctx) {
    unsigned long tmp = gettick();
    ctx->eax = (u32) tmp;
}

static void sys_open(struct esp_context *ctx) {
    int tmp = open((const char *)ctx->ebx, ctx->ecx);
    kSerialPrintf("open: %s (%d)\n", (char*)ctx->ebx, tmp);
    ctx->eax = (u32) tmp;
}

static void sys_read(struct esp_context *ctx) {
    s32 tmp = read(ctx->ebx, (void *)ctx->ecx, ctx->edx);
    ctx->eax = (u32) tmp;
}

static void sys_write(struct esp_context *ctx) {
    s32 tmp = write(ctx->ebx, (void *)ctx->ecx, ctx->edx);
    ctx->eax = (u32) tmp;
}

static void sys_seek(struct esp_context *ctx) {
    off_t tmp = seek(ctx->ebx, (off_t)ctx->ecx, ctx->edx);
    ctx->eax = (u32) tmp;
}

static void sys_close(struct esp_context *ctx) {
    int tmp = close(ctx->ebx);
    ctx->eax = (u32) tmp;
}

static void sys_setvideo(struct esp_context *ctx) {
    int tmp = switchVgaMode(ctx->ebx);
    ctx->eax = (u32) tmp;
}

static void sys_setVgaFrameBuffer(struct esp_context *ctx) {
    setVgaFrameBuffer((const void*)ctx->ebx);
    ctx->eax = 0;
}

static void sys_getMouse(struct esp_context *ctx) {
    ctx->eax = 0;
}

static void sys_playsound(struct esp_context *ctx) {
    ctx->eax = 0;
}

static void sys_getkeymode(struct esp_context *ctx) {
    ctx->eax = 0;
}

static void sys_sleep(struct esp_context *ctx) {
    ctx->eax = 0;
    taskAddEvent(TaskEventTimer, ctx->ebx);
}

static void sys_waitPid(struct esp_context *ctx) {
    ctx->eax = 0;
    taskAddEvent(TaskEventWaitPid, ctx->ebx);
}

static void sys_kill(struct esp_context *ctx) {
    ctx->eax = (u32) taskKillByPid(ctx->ebx);
}

static void sys_getPid(struct esp_context *ctx) {
    ctx->eax = taskGetpid();
}

