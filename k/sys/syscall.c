//
// Created by rebut_p on 23/09/18.
//

#include "io/serial.h"
#include "io/pit.h"
#include "io/terminal.h"
#include "syscall.h"
#include "io/libvga.h"
#include "physical-memory.h"

#include <kstdio.h>
#include <io/pipe.h>
#include <string.h>
#include <io/fs/ext2filesystem.h>

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define NB_SYSCALL 31

static u32 sys_write(struct esp_context *ctx);

static u32 sys_sbrk(struct esp_context *ctx);

static u32 sys_getkey(struct esp_context *ctx);

static u32 sys_gettick(struct esp_context *ctx);

static u32 sys_open(struct esp_context *ctx);

static u32 sys_read(struct esp_context *ctx);

static u32 sys_seek(struct esp_context *ctx);

static u32 sys_close(struct esp_context *ctx);

static u32 sys_setvideo(struct esp_context *ctx);

static u32 sys_setVgaFrameBuffer(struct esp_context *ctx);

static u32 sys_getMouse(struct esp_context *ctx);

static u32 sys_playsound(struct esp_context *ctx);

static u32 sys_getkeymode(struct esp_context *ctx);

static u32 sys_usleep(struct esp_context *ctx);

static u32 sys_waitPid(struct esp_context *ctx);

static u32 sys_exit(struct esp_context *ctx);

static u32 sys_kill(struct esp_context *ctx);

static u32 sys_getPid(struct esp_context *ctx);

static u32 sys_execve(struct esp_context *ctx);

static u32 sys_stat(struct esp_context *ctx);

static u32 sys_fstat(struct esp_context *ctx);

static u32 sys_chdir(struct esp_context *ctx);

static u32 sys_opendir(struct esp_context *ctx);

static u32 sys_closedir(struct esp_context *ctx);

static u32 sys_readdir(struct esp_context *ctx);

static u32 sys_mount(struct esp_context *ctx);

static u32 sys_umount(struct esp_context *ctx);

static u32 sys_pipe(struct esp_context *ctx);

static u32 sys_dup2(struct esp_context *ctx);

static u32 sys_getcwd(struct esp_context *ctx);

static u32 sys_sysconf(struct esp_context *ctx);

static u32 sys_touch(struct esp_context *ctx);

typedef u32 (*syscall_t)(struct esp_context *);

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
        sys_readdir,
        sys_mount,
        sys_umount,
        sys_pipe,
        sys_dup2,
        sys_getcwd,
        sys_sysconf,
        sys_touch
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
        klog("unhandled syscall %d\n", ctx->eax);
        ctx->eax = 0;
        return;
    }

    ctx->eax = fct(ctx);
}

/*** SYSCALL FCT ***/

static u32 sys_exit(struct esp_context *ctx) {
    (void) ctx;
    return (u32) taskExit();
}

static u32 sys_sbrk(struct esp_context *ctx) {
    return taskSetHeapInc((s32) ctx->ebx);
}

static u32 sys_getkey(struct esp_context *ctx) {
    (void) ctx;

    struct Kobject *kobject = taskGetKObjectByFd(0);
    if (kobject == NULL)
        return (u32) -1;

    return (u32) consoleGetkey2(kobject->data);
}

static u32 sys_gettick(struct esp_context *ctx) {
    (void) ctx;
    return (u32) gettick();
}

static u32 sys_open(struct esp_context *ctx) {
    int fd = taskGetAvailableFd(currentTask);
    if (fd == -1)
        return (u32) -1;

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return (u32) -1;

    taskSetKObjectByFd(fd, koCreate(KO_FS_FILE, path, ctx->ecx));
    LOG("open: %s (%d)\n", (char *) ctx->ebx, fd);
    return (u32) fd;
}

static u32 sys_read(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return (u32) -1;

    return (u32) koRead(obj, (void *) ctx->ecx, ctx->edx);
}

static u32 sys_write(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return (u32) -1;

    return (u32) koWrite(obj, (void *) ctx->ecx, ctx->edx);
}

static u32 sys_seek(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return (u32) -1;

    return (u32) koSeek(obj, (off_t) ctx->ecx, (int) ctx->edx);
}

static u32 sys_close(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return (u32) -1;

    currentTask->objectList[ctx->ebx] = NULL;
    return (u32) koDestroy(obj);
}

static u32 sys_setvideo(struct esp_context *ctx) {
    return (u32) consoleSwitchVideoMode(currentTask->console, (enum ConsoleMode) ctx->ebx);
}

static u32 sys_setVgaFrameBuffer(struct esp_context *ctx) {
    return (u32) consoleSetVgaFrameBuffer(currentTask->console, (const void *) ctx->ebx);
}

static u32 sys_getMouse(struct esp_context *ctx) {
    (void) ctx;
    return 0;
}

static u32 sys_playsound(struct esp_context *ctx) {
    (void) ctx;
    return 0;
}

static u32 sys_getkeymode(struct esp_context *ctx) {
    (void) ctx;
    return 0;
}

static u32 sys_usleep(struct esp_context *ctx) {
    taskWaitEvent(TaskEventTimer, ctx->ebx);
    return 0;
}

static u32 sys_waitPid(struct esp_context *ctx) {
    taskWaitEvent(TaskEventWaitPid, ctx->ebx);
    return 0;
}

static u32 sys_kill(struct esp_context *ctx) {
    return  (u32) taskKillByPid((pid_t) ctx->ebx);
}

static u32 sys_getPid(struct esp_context *ctx) {
    (void) ctx;
    return (u32) taskGetpid();
}

static u32 sys_execve(struct esp_context *ctx) {
    if (ctx->ebx == 0)
        return (u32) -1;

    LOG("execve\n");
    return (u32) createProcess((const struct ExceveInfo *) ctx->ebx);
}

static u32 sys_stat(struct esp_context *ctx) {
    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return (u32) -1;

    int tmp = fsStat(path, (void *) ctx->ecx);
    fsPathDestroy(path);

    return (u32) tmp;
}

static u32 sys_fstat(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return (u32) -1;

    return (u32) fsStat(obj->data, (void *) ctx->ecx);
}

static u32 sys_chdir(struct esp_context *ctx) {
    LOG("change dir: %s\n", (char *) ctx->ebx);
    return  (u32) taskChangeDirectory((void *) ctx->ebx);
}

static u32 sys_opendir(struct esp_context *ctx) {
    int fd = taskGetAvailableFd(currentTask);
    if (fd == -1)
        return (u32) -1;

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return (u32) -1;

    taskSetKObjectByFd(fd, koCreate(KO_FS_FOLDER, path, 0));
    LOG("opendir: %s (%d) refcount: %u\n", (char *) ctx->ebx, fd, path->refcount);
    return (u32) fd;
}

static u32 sys_readdir(struct esp_context *ctx) {
    LOG("readdir: %d\n", ctx->ebx);
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return 0;

    return (u32) fsPathReaddir(obj->data, (void *) ctx->ecx);
}

static u32 sys_closedir(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (!obj)
        return (u32) -1;

    LOG("closedir: %d\n", ctx->ebx);
    currentTask->objectList[ctx->ebx] = NULL;
    return (u32) koDestroy(obj);
}

static u32 sys_mount(struct esp_context *ctx) {
    klog("mount: %s (%s)\n", (char *) ctx->ebx, (char *) ctx->ecx);
    klog("mount: get fs by name\n");
    struct Fs *fs = fsGetFileSystemByName((const char *) ctx->ecx);
    if (!fs)
        return (u32) -1;

    klog("mount: get fspath\n");
    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return (u32) -1;

    klog("mount: create new volume\n");
    void *tmp = fsMountVolumeOn(path, fs, ctx->edx);
    fsPathDestroy(path);

    if (tmp == NULL)
        return (u32) -1;

    klog("mount: end\n");
    return 0;
}

static u32 sys_umount(struct esp_context *ctx) {
    LOG("umount: %s\n", (char *) ctx->ebx);

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return (u32) -1;

    LOG("umount: destroy volume\n");
    int tmp = fsUmountVolume(path);
    fsPathDestroy(path);

    LOG("umount end\n");
    return (u32) tmp;
}

static u32 sys_pipe(struct esp_context *ctx) {
    LOG("pipe\n");

    int pipe1 = taskGetAvailableFd(currentTask);
    if (pipe1 == -1)
        return (u32) -1;

    taskSetKObjectByFd(pipe1, (void*) 0x80);

    int pipe2 = taskGetAvailableFd(currentTask);
    if (pipe2 == -1)
        goto failure;

    int *fd = (int *) ctx->ebx;
    struct Pipe *pipe = pipeCreate();
    if (pipe == NULL)
        goto failure;

    taskSetKObjectByFd(pipe1, koCreate(KO_PIPE, pipeAddref(pipe), O_RDONLY));
    taskSetKObjectByFd(pipe2, koCreate(KO_PIPE, pipeAddref(pipe), O_WRONLY));

    fd[0] = pipe1;
    fd[1] = pipe2;

    LOG("pipe end [%d, %d]\n", pipe1, pipe2);
    return 0;

    failure:
    taskSetKObjectByFd(pipe1, NULL);
    return (u32) -1;
}

static u32 sys_dup2(struct esp_context *ctx) {
    LOG("dup2: %u -> %u\n", ctx->ebx, ctx->ecx);

    if (ctx->ebx >= MAX_NB_FILE || ctx->ecx >= MAX_NB_FILE)
        return (u32) -1;

    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (obj == NULL)
        return (u32) -1;

    struct Kobject *obj2 = taskGetKObjectByFd(ctx->ecx);
    if (obj2 != NULL)
        koDestroy(obj2);

    taskSetKObjectByFd(ctx->ecx, koDupplicate(obj));
    return 0;
}

static u32 sys_getcwd(struct esp_context *ctx) {
    LOG("getcwd: %u\n", ctx->ecx);
    char *tmp = (char *)ctx->ebx;
    strcpy(tmp, "/tmp");

    return ctx->ebx;
}

static u32 sys_sysconf(struct esp_context *ctx) {
    LOG("sysconf: %u\n", ctx->ebx);
    switch (ctx->ebx) {
        case _SC_PHYS_PAGES:
            return physmemGetTotalPages();
        case _SC_PAGESIZE:
            return PAGESIZE;
        default:
            return  (u32) -1;
    };
}

static u32 sys_touch(struct esp_context *ctx) {
    LOG("touch: %s\n", (const char *) ctx->ebx);
    return ext2Touch((const char *)ctx->ebx, currentTask->rootDir->volume->privateData);
}
