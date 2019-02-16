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
#include <io/device/fscache.h>
#include <errno-base.h>

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define NB_SYSCALL 38

static int sys_write(struct esp_context *ctx);

static int sys_sbrk(struct esp_context *ctx);

static int sys_getkey(struct esp_context *ctx);

static int sys_gettick(struct esp_context *ctx);

static int sys_open(struct esp_context *ctx);

static int sys_read(struct esp_context *ctx);

static int sys_seek(struct esp_context *ctx);

static int sys_close(struct esp_context *ctx);

static int sys_setvideo(struct esp_context *ctx);

static int sys_setVgaFrameBuffer(struct esp_context *ctx);

static int sys_getMouse(struct esp_context *ctx);

static int sys_playsound(struct esp_context *ctx);

static int sys_getkeymode(struct esp_context *ctx);

static int sys_usleep(struct esp_context *ctx);

static int sys_waitPid(struct esp_context *ctx);

static int sys_exit(struct esp_context *ctx);

static int sys_kill(struct esp_context *ctx);

static int sys_getPid(struct esp_context *ctx);

static int sys_execve(struct esp_context *ctx);

static int sys_stat(struct esp_context *ctx);

static int sys_fstat(struct esp_context *ctx);

static int sys_chdir(struct esp_context *ctx);

static int sys_opendir(struct esp_context *ctx);

static int sys_closedir(struct esp_context *ctx);

static int sys_readdir(struct esp_context *ctx);

static int sys_mount(struct esp_context *ctx);

static int sys_umount(struct esp_context *ctx);

static int sys_pipe(struct esp_context *ctx);

static int sys_dup2(struct esp_context *ctx);

static int sys_getcwd(struct esp_context *ctx);

static int sys_sysconf(struct esp_context *ctx);

static int sys_sync(struct esp_context *ctx);

static int sys_mkdir(struct esp_context *ctx);

static int sys_mkfile(struct esp_context *ctx);

static int sys_fchdir(struct esp_context *ctx);

static int sys_link(struct esp_context *ctx);

static int sys_symlink(struct esp_context *ctx);

static int sys_unlink(struct esp_context *ctx);

typedef int (*syscall_t)(struct esp_context *);

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
        sys_sync,
        sys_mkdir,
        sys_mkfile,
        sys_fchdir,
        sys_link,
        sys_symlink,
        sys_unlink
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
        ctx->eax = (u32) -EINTR;
        return;
    }

    ctx->eax = (u32) fct(ctx);
}

/*** SYSCALL FCT ***/

static int sys_exit(struct esp_context *ctx) {
    (void) ctx;
    return taskExit();
}

static int sys_sbrk(struct esp_context *ctx) {
    return taskSetHeapInc((s32) ctx->ebx);
}

static int sys_getkey(struct esp_context *ctx) {
    (void) ctx;

    struct Kobject *kobject = taskGetKObjectByFd(0);
    if (kobject == NULL)
        return -EBADF;

    return consoleGetkey2(kobject->data);
}

static int sys_gettick(struct esp_context *ctx) {
    (void) ctx;
    return (int) gettick();
}

static int sys_open(struct esp_context *ctx) {
    int fd = taskGetAvailableFd(currentTask);
    if (fd < 0)
        return fd;

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path) {
        if (!(ctx->ecx & O_CREAT))
            return -EACCES;

        path = fsLink((const char *) ctx->ebx, NULL);
        if (!path)
            return -EIO;
    }

    enum KObjectType type;
    void *data = fsOpenFile(path, ctx->edx, (int *) &type);
    if (data == NULL) {
        fsPathDestroy(path);
        return -EACCES;
    }


    struct Kobject *obj = koCreate(type, data, ctx->ecx);
    if (obj == NULL) {
        fsPathDestroy(path);
        return -ENOMEM;
    }

    taskSetKObjectByFd(fd, obj);
    LOG("open: %s (%d)\n", (char *) ctx->ebx, fd);
    if (ctx->ecx & O_APPEND)
        obj->offset = path->size;
    else if (ctx->ecx & O_TRUNC) {
        fsResizeFile(path, 0);
    }

    return fd;
}

static int sys_read(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return -EBADF;

    return koRead(obj, (void *) ctx->ecx, ctx->edx);
}

static int sys_write(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return -EBADF;

    return koWrite(obj, (void *) ctx->ecx, ctx->edx);
}

static int sys_seek(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return -EBADF;

    return koSeek(obj, (off_t) ctx->ecx, (int) ctx->edx);
}

static int sys_close(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return -EBADF;

    currentTask->objectList[ctx->ebx] = NULL;
    return koDestroy(obj);
}

static int sys_setvideo(struct esp_context *ctx) {
    return consoleSwitchVideoMode(currentTask->console, (enum ConsoleMode) ctx->ebx);
}

static int sys_setVgaFrameBuffer(struct esp_context *ctx) {
    return consoleSetVgaFrameBuffer(currentTask->console, (const void *) ctx->ebx);
}

static int sys_getMouse(struct esp_context *ctx) {
    (void) ctx;
    return 0;
}

static int sys_playsound(struct esp_context *ctx) {
    (void) ctx;
    return 0;
}

static int sys_getkeymode(struct esp_context *ctx) {
    (void) ctx;
    return 0;
}

static int sys_usleep(struct esp_context *ctx) {
    taskWaitEvent(TaskEventTimer, ctx->ebx);
    return 0;
}

static int sys_waitPid(struct esp_context *ctx) {
    taskWaitEvent(TaskEventWaitPid, ctx->ebx);
    return 0;
}

static int sys_kill(struct esp_context *ctx) {
    return taskKillByPid((pid_t) ctx->ebx);
}

static int sys_getPid(struct esp_context *ctx) {
    (void) ctx;
    return taskGetpid();
}

static int sys_execve(struct esp_context *ctx) {
    if (ctx->ebx == 0)
        return -EINVAL;

    LOG("execve\n");
    return createProcess((const struct ExceveInfo *) ctx->ebx);
}

static int sys_stat(struct esp_context *ctx) {
    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return -ENOENT;

    int tmp = fsStat(path, (void *) ctx->ecx);
    fsPathDestroy(path);

    return tmp;
}

static int sys_fstat(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return -EBADF;

    return fsStat(obj->data, (void *) ctx->ecx);
}

static int sys_chdir(struct esp_context *ctx) {
    LOG("change dir: %s\n", (char *) ctx->ebx);
    return taskChangeDirectory((void *) ctx->ebx);
}

static int sys_opendir(struct esp_context *ctx) {
    int fd = taskGetAvailableFd(currentTask);
    if (fd < 0)
        return fd;

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return -ENOENT;

    taskSetKObjectByFd(fd, koCreate(KO_FS_FOLDER, path, 0));
    LOG("opendir: %s (%d) refcount: %u\n", (char *) ctx->ebx, fd, path->refcount);
    return fd;
}

static int sys_readdir(struct esp_context *ctx) {
    LOG("readdir: %d (%d)\n", ctx->ebx, ctx->edx);
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return -EBADF;

    return fsPathReaddir(obj->data, (void *) ctx->ecx, ctx->edx);
}

static int sys_closedir(struct esp_context *ctx) {
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj)
        return -EBADF;

    LOG("closedir: %d\n", ctx->ebx);
    currentTask->objectList[ctx->ebx] = NULL;
    return koDestroy(obj);
}

static int sys_mount(struct esp_context *ctx) {
    klog("mount %s on %s (%s)\n", (char *) ctx->ecx, (char *) ctx->edx, (char *) ctx->ebx);
    klog("mount: get fs by name\n");
    struct Fs *fs = fsGetFileSystemByName((const char *) ctx->ebx);
    if (!fs)
        return -ENODEV;

    klog("mount: get fspath\n");
    struct FsPath *path = fsResolvePath((char *) ctx->edx);
    if (!path)
        return -ENOENT;

    klog("mount: get device path\n");
    struct FsPath *device = fsResolvePath((char *) ctx->ecx);
    if (device == NULL) {
        fsPathDestroy(path);
        return -ENXIO;
    }

    klog("mount: create new volume\n");
    void *tmp = fsMountVolumeOn(path, fs, device);
    fsPathDestroy(path);
    fsPathDestroy(device);

    if (tmp == NULL)
        return -EIO;

    klog("mount: end\n");
    return 0;
}

static int sys_umount(struct esp_context *ctx) {
    LOG("umount: %s\n", (char *) ctx->ebx);

    struct FsPath *path = fsResolvePath((char *) ctx->ebx);
    if (!path)
        return -ENOENT;

    LOG("umount: destroy volume\n");
    int tmp = fsUmountVolume(path);
    fsPathDestroy(path);

    LOG("umount end\n");
    return tmp;
}

static int sys_pipe(struct esp_context *ctx) {
    LOG("pipe\n");

    int pipe1 = taskGetAvailableFd(currentTask);
    if (pipe1 < 0)
        return -pipe1;

    taskSetKObjectByFd(pipe1, (void *) 0x80);

    int pipe2 = taskGetAvailableFd(currentTask);
    if (pipe2 < 0) {
        taskSetKObjectByFd(pipe1, NULL);
        return -pipe2;
    }

    int *fd = (int *) ctx->ebx;
    struct Pipe *pipe = pipeCreate();
    if (pipe == NULL) {
        taskSetKObjectByFd(pipe1, NULL);
        return -ENOMEM;
    }

    taskSetKObjectByFd(pipe1, koCreate(KO_PIPE, pipeAddref(pipe), O_RDONLY));
    taskSetKObjectByFd(pipe2, koCreate(KO_PIPE, pipeAddref(pipe), O_WRONLY));

    fd[0] = pipe1;
    fd[1] = pipe2;

    LOG("pipe end [%d, %d]\n", pipe1, pipe2);
    return 0;
}

static int sys_dup2(struct esp_context *ctx) {
    LOG("dup2: %u -> %u\n", ctx->ebx, ctx->ecx);

    if (ctx->ebx >= MAX_NB_FILE || ctx->ecx >= MAX_NB_FILE)
        return -EBADF;

    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (obj == NULL)
        return -EBADF;

    struct Kobject *obj2 = taskGetKObjectByFd(ctx->ecx);
    if (obj2 != NULL)
        koDestroy(obj2);

    taskSetKObjectByFd(ctx->ecx, koDupplicate(obj));
    return 0;
}

static int sys_getcwd(struct esp_context *ctx) {
    LOG("getcwd: %u\n", ctx->ecx);
    char *tmp = (char *) ctx->ebx;
    strcpy(tmp, "/tmp");

    return 0;
}

static int sys_sysconf(struct esp_context *ctx) {
    LOG("sysconf: %u\n", ctx->ebx);
    switch (ctx->ebx) {
        case _SC_PHYS_PAGES:
            return physmemGetTotalPages();
        case _SC_PAGESIZE:
            return PAGESIZE;
        default:
            return -EPERM;
    };
}

static int sys_sync(struct esp_context *ctx) {
    (void) ctx;
    LOG("sync\n");
    fsCacheSync();
    return 0;
}

static int sys_mkdir(struct esp_context *ctx) {
    LOG("mkdir: %s\n", (char *) ctx->ebx);

    void *tmp = fsMkdir((const char *) ctx->ebx);
    return (tmp ? 0 : -EPERM);
}

static int sys_mkfile(struct esp_context *ctx) {
    LOG("mkfile: %s\n", (char *) ctx->ebx);

    void *tmp = fsResolvePath((const char *) ctx->ebx);
    if (tmp) {
        fsPathDestroy(tmp);
        return -EEXIST;
    }

    tmp = fsLink((const char *) ctx->ebx, NULL);
    fsPathDestroy(tmp);
    return (tmp ? 0 : -EIO);
}

static int sys_fchdir(struct esp_context *ctx) {
    LOG("fchdir: %d\n", ctx->ebx);
    struct Kobject *obj = taskGetKObjectByFd(ctx->ebx);
    if (!obj || obj->type != KO_FS_FOLDER)
        return -EBADF;

    fsPathDestroy(currentTask->currentDir);

    currentTask->currentDir = obj->data;
    currentTask->currentDir->refcount += 1;

    return 0;
}

static int sys_link(struct esp_context *ctx) {
    LOG("link: %s -> %s\n", (char *) ctx->ebx, (char *) ctx->ecx);

    void *tmp = fsResolvePath((const char *) ctx->ecx);
    if (tmp) {
        fsPathDestroy(tmp);
        return -EEXIST;
    }

    tmp = fsLink((const char *) ctx->ecx, (const char *) ctx->ebx);
    fsPathDestroy(tmp);
    return (tmp ? 0 : -EIO);
}

static int sys_symlink(struct esp_context *ctx) {
    (void) ctx; // todo

    LOG("symlink: %s -> %s\n", (char *) ctx->ebx, (char *) ctx->ecx);

    return -EPERM;
}

static int sys_unlink(struct esp_context *ctx) {
    (void) ctx; // todo
    LOG("unlink: %s\n", (char *) ctx->ebx);

    return -EPERM;
}