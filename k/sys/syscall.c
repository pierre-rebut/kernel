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

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define NB_SYSCALL 25

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
static void sys_usleep(struct esp_context *ctx);
static void sys_waitPid(struct esp_context *ctx);
static void sys_exit(struct esp_context *ctx);
static void sys_kill(struct esp_context *ctx);
static void sys_getPid(struct esp_context *ctx);
static void sys_execve(struct esp_context *ctx);
static void sys_stat(struct esp_context *ctx);
static void sys_fstat(struct esp_context *ctx);
static void sys_chdir(struct esp_context *ctx);
static void sys_opendir(struct esp_context *ctx);
static void sys_closedir(struct esp_context *ctx);
static void sys_readdir(struct esp_context *ctx);

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
        sys_usleep,
        sys_waitPid,
        sys_kill,
        sys_getPid,
        sys_execve,
        sys_stat,
        sys_fstat,
        sys_chdir,
        sys_getkeymode,
        sys_opendir,
        sys_closedir,
        sys_readdir
};

static void syscall_handler(struct esp_context *ctx);

void initSyscall() {
    interruptRegister(128, &syscall_handler);
}

static void syscall_handler(struct esp_context *ctx) {
    if (ctx->eax >= NB_SYSCALL)
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
    int tmp = consoleGetkey(currentTask->console);
    ctx->eax = (u32) tmp;
}

static void sys_gettick(struct esp_context *ctx) {
    unsigned long tmp = gettick();
    ctx->eax = (u32) tmp;
}

static void sys_open(struct esp_context *ctx) {
    int tmp = open((const char *)ctx->ebx, ctx->ecx);
    LOG("open: %s (%d)\n", (char*)ctx->ebx, tmp);
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
    LOG("close: %d\n", ctx->ebx);
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

static void sys_usleep(struct esp_context *ctx) {
    ctx->eax = 0;
    taskWaitEvent(TaskEventTimer, ctx->ebx);
}

static void sys_waitPid(struct esp_context *ctx) {
    ctx->eax = 0;
    taskWaitEvent(TaskEventWaitPid, ctx->ebx);
}

static void sys_kill(struct esp_context *ctx) {
    ctx->eax = taskKillByPid(ctx->ebx);
}

static void sys_getPid(struct esp_context *ctx) {
    ctx->eax = taskGetpid();
}

static void sys_execve(struct esp_context *ctx) {

    if (ctx->ebx == 0 || ctx->ecx == 0 || ctx->edx == 0) {
        ctx->eax = 0;
        return;
    }

    LOG("execve: %s\n", (char*)ctx->ebx);
    ctx->eax = createProcess((const char*)ctx->ebx, (const char**)ctx->ecx, (const char **)ctx->edx);
}

static void sys_stat(struct esp_context *ctx) {
    s32 tmp = stat((void*)ctx->ebx, (void *)ctx->ecx);
    LOG("stat: %s (%d)\n", (char*)ctx->ebx, tmp);
    ctx->eax = (u32) tmp;
}

static void sys_fstat(struct esp_context *ctx) {
    s32 tmp = fstat(ctx->ebx, (void*)ctx->ecx);
    ctx->eax = (u32) tmp;
}

static void sys_chdir(struct esp_context *ctx) {
    s32 tmp = taskChangeDirectory((void*)ctx->ebx);
    ctx->eax = (u32) tmp;
}

static void sys_opendir(struct esp_context *ctx) {
    int tmp = opendir((const char *)ctx->ebx);
    LOG("opendir: %s (%d)\n", (char*)ctx->ebx, tmp);
    ctx->eax = (u32) tmp;
}

static void sys_readdir(struct esp_context *ctx) {
    ctx->eax = (u32) readdir(ctx->ebx, (void*)ctx->ecx);
}

static void sys_closedir(struct esp_context *ctx) {
    LOG("closedir: %d\n", ctx->ebx);
    int tmp = closedir(ctx->ebx);
    ctx->eax = (u32) tmp;
}

