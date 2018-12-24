//
// Created by rebut_p on 21/12/18.
//

#include <io/fs/filesystem.h>
#include <sys/allocator.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <stdlib.h>
#include <sheduler.h>
#include <sys/physical-memory.h>
#include <sys/time.h>
#include <io/pit.h>
#include "procfilesystem.h"

#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
//#define LOG(x, ...)

enum ProcPathType {
    PP_FILE,
    PP_FOLDER
};

struct ProcPath {
    enum ProcPathType type;
    int data;
};

static struct FsPath *procRoot(struct FsVolume *volume) {
    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "procRoot");
    struct ProcPath *procPath = kmalloc(sizeof(struct ProcPath), 0, "newProcPath");

    if (!procPath || !rootPath) {
        kfree(rootPath);
        kfree(procPath);
        return NULL;
    }

    (void) volume;

    procPath->type = PP_FOLDER;
    procPath->data = -1;
    rootPath->privateData = procPath;
    return rootPath;
}

static struct FsVolume *procMount(void *data) {
    LOG("[proc] mount:\n");
    (void) data;

    struct FsVolume *procVolume = kmalloc(sizeof(struct FsVolume), 0, "newProcVolume");
    if (procVolume == NULL)
        return NULL;

    procVolume->blockSize = PROC_BLK_DATA_SZ;
    procVolume->privateData = NULL;
    return procVolume;
}

static int procUmount(struct FsVolume *volume) {
    kfree(volume);
    return 0;
}

static int procClose(struct FsPath *path) {
    kfree(path->privateData);
    kfree(path);
    return 0;
}

static int procStat(struct FsPath *path, struct stat *result) {
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;
    if (procPath->type != PP_FILE)
        return -1;

    result->inumber = (u32) procPath->data;
    result->file_sz = 0;
    result->i_blk_cnt = 0;
    result->d_blk_cnt = 0;
    result->blk_cnt = 0;
    result->idx = 0;
    result->cksum = 0;
    return 0;
}

static struct FsPath *procLookup(struct FsPath *path, const char *name) {
    (void)path;

    struct ProcPath *procPath = kmalloc(sizeof(struct ProcPath), 0, "newProcPath");
    if (procPath == NULL)
        return NULL;

    if (*name == 0 || strcmp(name, ".") == 0) {
        procPath->data = -4;
        procPath->type = PP_FOLDER;
    } else if (strcmp(name, "mounts") == 0) {
        procPath->data = -1;
        procPath->type = PP_FILE;
    } else if (strcmp(name, "meminfo") == 0) {
        procPath->data = -2;
        procPath->type = PP_FILE;
    } else if (strcmp(name, "time") == 0) {
        procPath->data = -3;
        procPath->type = PP_FILE;
    } else if (strcmp(name, "uptime") == 0) {
        procPath->data = -4;
        procPath->type = PP_FILE;
    } else {
        struct Task *task;

        int pid = atoi(name);
        if (pid == 0)
            task = &kernelTask;
        else
            task = getTaskByPid((u32) pid);

        if (task == NULL) {
            kfree(procPath);
            return NULL;
        }

        procPath->type = PP_FILE;
        procPath->data = task->pid;
    }

    LOG("[proc] lookup enter\n");
    struct FsPath *file = kmalloc(sizeof(struct FsPath), 0, "FsPath");
    if (!file) {
        kfree(procPath);
        return NULL;
    }

    file->privateData = procPath;
    file->size = 0;
    return file;
}

static struct dirent *procReaddir(struct FsPath *path, struct dirent *result) {
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;

    LOG("[proc] readdir\n");
    if (!procPath || procPath->type != PP_FOLDER)
        return NULL;

    if (procPath->data < 0) {
        switch (procPath->data) {
            case -1:
                ksprintf(result->d_name, "%s", "mounts");
                break;
            case -2:
                ksprintf(result->d_name, "%s", "meminfo");
                break;
            case -3:
                ksprintf(result->d_name, "%s", "time");
                break;
            case -4:
                ksprintf(result->d_name, "%s", "uptime");
                break;
            default:
                return NULL;
        }
        result->d_type = FT_FILE;
        result->d_ino = (u32) procPath->data;
    } else {
        struct Task *task = schedulerGetTaskByIndex((u32) procPath->data);
        if (!task)
            return NULL;

        ksprintf(result->d_name, "%u", task->pid);
        result->d_ino = (u32) procPath->data;
        result->d_type = FT_FILE;
    }

    procPath->data++;
    return result;
}

static int procReadBlock(struct FsPath *path, char *buffer, u32 blocknum) {
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;
    LOG("[proc] readblock: %u\n", blocknum);
    if (!procPath || procPath->type != PP_FILE || blocknum > 0)
        return -1;

    int read;
    if (procPath->data < 0) {
        switch (procPath->data) {
            case -1:
                read = 0;
                struct FsVolume *tmpVolume = fsVolumeList;
                while (tmpVolume) {
                    read += ksprintf(buffer + read, "[VOLUME]\nmount:%c\ntype:%s\ndevice:%X\nrefcount:%u\nblocksize:%u\n",
                                     tmpVolume->id, tmpVolume->fs->name, tmpVolume->privateData, tmpVolume->refcount,
                                     tmpVolume->blockSize
                    );
                    tmpVolume = tmpVolume->next;
                    if (tmpVolume != NULL)
                        buffer[read++] = '\n';
                }
                break;
            case -2:
                read = ksprintf(buffer, "[PHYSMEM]\nused:%u\ntotal:%u\n\n[KERNEL MEM]\nused:%u\npaged:%u\n",
                                getTotalUsedPhysMemory(), getTotalPhysMemory(), getTotalUsedAlloc(), getTotalPagedAlloc()
                );
                break;
            case -3:
                read = getCurrentDateAndTime(buffer);
                buffer[read++] = '\n';
                buffer[read] = '\0';
                break;
            case -4:
                read = ksprintf(buffer, "%lu\n", gettick());
                break;
            default:
                read = -1;
        }
    } else {
        struct Task *task = getTaskByPid((u32) procPath->data);
        if (!task)
            return -1;

        read = ksprintf(buffer, "cmdline:%s\nprivilege:%s\nevent:%d,%lu,%u\n",
                            (task->cmdline ? task->cmdline : "NONE"),
                            (task->privilege == TaskPrivilegeKernel ? "KERNEL" : "USER"),
                            task->event.type, task->event.timer, task->event.arg
        );
    }

    return read;
}

static struct Fs fs_procfs = {
        .name = "proc",
        .mount = &procMount,
        .umount = &procUmount,
        .root = &procRoot,
        .lookup = &procLookup,
        .readdir = &procReaddir,
        .readBlock = &procReadBlock,
        .stat = &procStat,
        .close = &procClose
};

void initProcFileSystem() {
    fsRegister(&fs_procfs);
}