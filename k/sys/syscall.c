//
// Created by rebut_p on 23/09/18.
//

#include "io/serial.h"
#include "io/keyboard.h"
#include "io/pit.h"
#include "io/fs/kfilesystem.h"
#include "io/terminal.h"
#include "syscall.h"
#include "io/libvga.h"
#include "task.h"
#include "allocator.h"

#include <stdio.h>

#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
//#define LOG(x, ...)

#define NB_SYSCALL 28

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

static void sys_mount2(struct esp_context *ctx);

static void sys_umount(struct esp_context *ctx);

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
        sys_mount2
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
    ctx->eax = taskSetHeapInc((s32) ctx->ebx);
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

    currentTask->objectList[fd] = koCreate(KO_FS, path, ctx->ecx);
    ctx->eax = (u32) fd;
    LOG("open: %s (%d)\n", (char *) ctx->ebx, fd);
}

static void sys_read(struct esp_context *ctx) {
    struct Kobject *obj = currentTask->objectList[ctx->ebx];

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }
    ctx->eax = (u32) koRead(obj, (void *) ctx->ecx, ctx->edx);
}

static void sys_write(struct esp_context *ctx) {
    struct Kobject *obj = currentTask->objectList[ctx->ebx];

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }
    ctx->eax = (u32) koWrite(obj, (void *) ctx->ecx, ctx->edx);
}

static void sys_seek(struct esp_context *ctx) {
    struct Kobject *obj = currentTask->objectList[ctx->ebx];

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }

    ctx->eax = (u32) koSeek(obj, (off_t)ctx->ecx, (int)ctx->edx);
}

static void sys_close(struct esp_context *ctx) {
    struct Kobject *obj = currentTask->objectList[ctx->ebx];

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }
    ctx->eax = (u32) koDestroy(obj);
    currentTask->objectList[ctx->ebx] = NULL;
}

static void sys_setvideo(struct esp_context *ctx) {
    ctx->eax = (u32) switchVgaMode((enum ConsoleMode) ctx->ebx);
}

static void sys_setVgaFrameBuffer(struct esp_context *ctx) {
    setVgaFrameBuffer((const void *) ctx->ebx);
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

    LOG("execve: %s\n", (char *) ctx->ebx);
    ctx->eax = createProcess((const char *) ctx->ebx, (const char **) ctx->ecx, (const char **) ctx->edx);
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
    struct Kobject *obj = currentTask->objectList[ctx->ebx];

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

    currentTask->objectList[fd] = koCreate(KO_FS_FOLDER, path, 0);
    LOG("opendir: %s (%d) refcount: %u\n", (char *) ctx->ebx, fd, path->refcount);
    ctx->eax = (u32) fd;
}

static void sys_readdir(struct esp_context *ctx) {
    LOG("readdir: %d\n", ctx->ebx);
    struct Kobject *obj = currentTask->objectList[ctx->ebx];
    if (!obj) {
        ctx->eax = 0;
        return;
    }

    ctx->eax = (u32) fsPathReaddir(obj->data, (void *) ctx->ecx);
}

static void sys_closedir(struct esp_context *ctx) {
    struct Kobject *obj = currentTask->objectList[ctx->ebx];

    if (!obj) {
        ctx->eax = (u32) -1;
        return;
    }

    LOG("closedir: %d\n", ctx->ebx);
    ctx->eax = (u32) koDestroy(obj);
    currentTask->objectList[ctx->ebx] = NULL;
}

static void sys_mount(struct esp_context *ctx) {
    LOG("mount: %s -> %c (%s)\n", (char *) ctx->edx, ctx->ebx, (char *) ctx->ecx);

    char *data = NULL;
    struct FsPath *file = NULL;

    if (fsGetVolumeById((char)ctx->ebx))
        goto failure;

    LOG("mount: get fs by name\n");
    struct Fs *fs = fsGetFileSystemByName((const char *) ctx->ecx);
    if (!fs)
        goto failure;

    LOG("mount: resolve path\n");
    file = fsResolvePath((const char *)ctx->edx);
    if (!file)
        goto failure;

    LOG("mount: alloc memory %u\n", file->size);
    data = kmalloc(sizeof(char) * file->size, 0, "mountAlloc");
    if (!data)
        goto failure;

    LOG("mount: read file\n");
    if (fsReadFile(file, data, file->size, 0) != (s32) file->size)
        goto failure;

    LOG("mount: create new volume\n");
    struct FsVolume *volume = fsVolumeOpen((char)ctx->ebx, fs, data);
    if (!volume)
        goto failure;

    fsPathDestroy(file);
    ctx->eax = 0;
    LOG("mount: end\n");
    return;

    failure:
    kSerialPrintf("mount: failure\n");
    fsPathDestroy(file);
    kfree(data);
    ctx->eax = (u32) -1;
}

static void sys_mount2(struct esp_context *ctx) {
    LOG("mount2: %d -> %c (%s)\n", ctx->edx, ctx->ebx, (char *) ctx->ecx);

    if (fsGetVolumeById((char)ctx->ebx))
        goto failure;

    LOG("mount2: get fs by name\n");
    struct Fs *fs = fsGetFileSystemByName((const char *) ctx->ecx);
    if (!fs)
        goto failure;

    LOG("mount2: create new volume\n");
    struct FsVolume *volume = fsVolumeOpen((char)ctx->ebx, fs, (void*)ctx->edx);
    if (!volume)
        goto failure;
    
    ctx->eax = 0;
    LOG("mount2: end\n");
    return;

    failure:
    kSerialPrintf("mount2: failure\n");
    ctx->eax = (u32) -1;
}

static void sys_umount(struct esp_context *ctx) {
    LOG("umount: %c\n", ctx->ebx);

    struct FsVolume *volume = fsGetVolumeById((char)ctx->ebx);
    if (!volume)
        goto failure;

    LOG("umount: destroy volume\n");

    if (fsVolumeClose(volume) == -1)
        goto failure;

    LOG("umount end\n");
    ctx->eax = 0;
    return;

    failure:
    kSerialPrintf("umount: failed\n");
    ctx->eax = (u32) -1;
}

