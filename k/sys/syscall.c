//
// Created by rebut_p on 23/09/18.
//

#include "io/serial.h"
#include "io/pit.h"
#include "io/terminal.h"
#include "syscall.h"
#include "io/libvga.h"

#include <kstdio.h>
#include <io/pipe.h>

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define NB_SYSCALL 30

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

static void sys_mount(struct esp_context *ctx);

static void sys_umount(struct esp_context *ctx);

static void sys_pipe(struct esp_context *ctx);

static void sys_dup2(struct esp_context *ctx);

static void sys_getcwd(struct esp_context *ctx);

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
        sys_readdir,
        sys_mount,
        sys_umount,
        sys_pipe,
        sys_dup2,
        sys_getcwd
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

    fct(ctx);
}

/*** SYSCALL FCT ***/

static void sys_exit(struct esp_context *ctx) {
    ctx->eax = (u32) taskExit();
}

static void sys_sbrk(struct esp_context *ctx) {
    ctx->eax = taskSetHeapInc((s32) ctx->ebx);
}

static void sys_getkey(struct esp_context *ctx) {
    struct Kobject *kobject = taskGetKObjectByFd(0);
    if (kobject == NULL) {
        ctx->eax = (u32) -1;
        return;
    }

    int tmp = consoleGetkey2(kobject->data);
    ctx->eax = (u32) tmp;
}

static void sys_gettick(struct esp_context *ctx) {
    unsigned long tmp = gettick();
    ctx->eax = (u32) tmp;
}

static void sys_open(struct esp_context *ctx) {
    int fd = taskGetAvailableFd(currentTask);
    if (fd == -1) {
        ctx->eax = (u32) -1;
        return;
    }

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path) {
        ctx->eax = (u32) -1;
        return;
    }

    taskSetKObjectByFd(fd, koCreate(KO_FS_FILE, path, ctx->ecx));
    ctx->eax = (u32) fd;
    LOG("open: %s (%d)\n", (char *) ctx->ebx, fd);
}

static void sys_read(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }
    ctx->eax = (u32) koRead(obj, (void *) ctx->ecx, ctx->edx);
}

static void sys_write(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }
    ctx->eax = (u32) koWrite(obj, (void *) ctx->ecx, ctx->edx);
}

static void sys_seek(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }

    ctx->eax = (u32) koSeek(obj, (off_t) ctx->ecx, (int) ctx->edx);
}

static void sys_close(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }
    ctx->eax = (u32) koDestroy(obj);
    currentTask->objectList[ctx->ebx] = NULL;
}

static void sys_setvideo(struct esp_context *ctx) {
    ctx->eax = (u32) consoleSwitchVideoMode(currentTask->console, (enum ConsoleMode) ctx->ebx);
}

static void sys_setVgaFrameBuffer(struct esp_context *ctx) {
    ctx->eax = (u32) consoleSetVgaFrameBuffer(currentTask->console, (const void *) ctx->ebx);
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
    ctx->eax = (u32) taskKillByPid((pid_t) ctx->ebx);
}

static void sys_getPid(struct esp_context *ctx) {
    ctx->eax = (u32) taskGetpid();
}

static void sys_execve(struct esp_context *ctx) {

    if (ctx->ebx == 0) {
        ctx->eax = 0;
        return;
    }

    LOG("execve\n");
    ctx->eax = (u32) createProcess((const struct ExceveInfo *) ctx->ebx);
}

static void sys_stat(struct esp_context *ctx) {
    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path) {
        ctx->eax = (u32) -1;
        return;
    }

    ctx->eax = (u32) fsStat(path, (void *) ctx->ecx);
    fsPathDestroy(path);
}

static void sys_fstat(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }
    ctx->eax = (u32) fsStat(obj->data, (void *) ctx->ecx);
}

static void sys_chdir(struct esp_context *ctx) {
    ctx->eax = (u32) taskChangeDirectory((void *) ctx->ebx);
}

static void sys_opendir(struct esp_context *ctx) {
    int fd = taskGetAvailableFd(currentTask);
    if (fd == -1) {
        ctx->eax = (u32) -1;
        return;
    }

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path) {
        ctx->eax = (u32) -1;
        return;
    }

    taskSetKObjectByFd(fd, koCreate(KO_FS_FOLDER, path, 0));
    LOG("opendir: %s (%d) refcount: %u\n", (char *) ctx->ebx, fd, path->refcount);
    ctx->eax = (u32) fd;
}

static void sys_readdir(struct esp_context *ctx) {
    LOG("readdir: %d\n", ctx->ebx);
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj) {
        ctx->eax = 0;
        return;
    }

    ctx->eax = (u32) fsPathReaddir(obj->data, (void *) ctx->ecx);
}

static void sys_closedir(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }

    LOG("closedir: %d\n", ctx->ebx);
    ctx->eax = (u32) koDestroy(obj);
    currentTask->objectList[ctx->ebx] = NULL;
}

static void sys_mount(struct esp_context *ctx) {
    klog("mount: %s (%s)\n", (char *)ctx->ebx, (char *) ctx->ecx);
    klog("mount: get fs by name\n");
    struct Fs *fs = fsGetFileSystemByName((const char *) ctx->ecx);
    if (!fs)
        goto failure;

    klog("mount: get fspath\n");
    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        goto failure;

    klog("mount: create new volume\n");
    void *tmp = fsMountVolumeOn(path, fs, ctx->edx);
    fsPathDestroy(path);

    if (tmp == NULL)
        goto failure;

    ctx->eax = 0;
    klog("mount: end\n");
    return;

    failure:
    klog("mount: failure\n");
    ctx->eax = (u32) -1;
}

static void sys_umount(struct esp_context *ctx) {
    LOG("umount: %s\n", (char *) ctx->ebx);

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        goto failure;

    LOG("umount: destroy volume\n");
    ctx->eax = (u32) fsUmountVolume(path);
    fsPathDestroy(path);

    LOG("umount end\n");
    return;

    failure:
    klog("umount: failed\n");
    ctx->eax = (u32) -1;
}

static void sys_pipe(struct esp_context *ctx) {
    LOG("pipe\n");

    int pipe1 = taskGetAvailableFd(currentTask);
    int pipe2 = taskGetAvailableFd(currentTask);
    if (pipe1 == -1 || pipe2 == -1)
        goto failure;

    int *fd = (int *) ctx->ebx;
    struct Pipe *pipe = pipeCreate();
    if (pipe == NULL)
        goto failure;

    taskSetKObjectByFd(pipe1, koCreate(KO_PIPE, pipeAddref(pipe), O_RDONLY));
    taskSetKObjectByFd(pipe2, koCreate(KO_PIPE, pipeAddref(pipe), O_WRONLY));

    fd[0] = pipe1;
    fd[1] = pipe2;

    ctx->eax = 0;
    return;

    failure:
    klog("pipe: failed\n");
    ctx->eax = (u32) -1;
}

static void sys_dup2(struct esp_context *ctx) {
    LOG("dup2: %u -> %u\n", ctx->ebx, ctx->ecx);

    if (ctx->ebx >= MAX_NB_FILE || ctx->ecx >= MAX_NB_FILE)
        goto failure;

    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);

    if (obj == NULL)
        goto failure;

    struct Kobject *obj2 = taskGetKObjectByFd(ctx->ecx);

    if (obj2 != NULL)
        koDestroy(obj2);

    taskSetKObjectByFd(ctx->ecx, koDupplicate(obj));
    ctx->eax = 0;
    return;

    failure:
    klog("dup2: failed\n");
    ctx->eax = (u32) -1;
}

static void sys_getcwd(struct esp_context *ctx) {
    LOG("getcwd: %u\n", ctx->ecx);
    ctx->eax = 0;
}

