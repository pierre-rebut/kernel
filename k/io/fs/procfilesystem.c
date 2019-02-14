//
// Created by rebut_p on 21/12/18.
//

#include <io/fs/filesystem.h>
#include <sys/allocator.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <stdlib.h>
#include <sys/physical-memory.h>
#include <sys/time.h>
#include <io/pit.h>
#include "procfilesystem.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

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
    rootPath->inode = 0;
    return rootPath;
}

static struct FsVolume *procMount(u32 data) {
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

    result->st_ino = (u32) procPath->data;
    result->st_size = 0;
    result->st_mode = 0;
    result->st_blksize = path->volume->blockSize;

    result->st_gid = result->st_uid = result->st_nlink = 0;
    result->st_atim = result->st_ctim = result->st_mtim = 0;

    return 0;
}

static struct FsPath *procLookup(struct FsPath *path, const char *name) {
    (void) path;

    struct ProcPath *procPath = kmalloc(sizeof(struct ProcPath), 0, "newProcPath");
    if (procPath == NULL)
        return NULL;

    if (*name == 0 || strcmp(name, ".") == 0) {
        procPath->data = -5;
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
    } else if (strcmp(name, "self") == 0) {
        procPath->data = -5;
        procPath->type = PP_FILE;
    } else {
        struct Task *task;

        int pid = atoi(name);
        if (pid == 0)
            task = &kernelTask;
        else
            task = getTaskByPid(pid);

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
    file->inode = 0;
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
                sprintf(result->d_name, "%s", "mounts");
                break;
            case -2:
                sprintf(result->d_name, "%s", "meminfo");
                break;
            case -3:
                sprintf(result->d_name, "%s", "time");
                break;
            case -4:
                sprintf(result->d_name, "%s", "uptime");
                break;
            case -5:
                sprintf(result->d_name, "%s", "self");
                break;
            default:
                return NULL;
        }
        result->d_type = FT_FILE;
        result->d_namlen = strlen(result->d_name);
        result->d_ino = (u32) procPath->data;
        procPath->data++;
    } else {
        struct Task *task = NULL;
        while ((u32) procPath->data < 1024 && task == NULL) {
            task = getTaskByPid((pid_t) procPath->data);
            procPath->data += 1;
        }

        if (!task)
            return NULL;

        sprintf(result->d_name, "%u", task->pid);
        result->d_ino = (u32) procPath->data;
        result->d_type = FT_FILE;
        result->d_namlen = strlen(result->d_name);
    }

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
                read = sprintf(buffer, "%s,%u,%u\n", fsRootVolume->fs->name, fsRootVolume->refcount,
                               fsRootVolume->blockSize);
                struct FsMountVolume *tmpMntVolume = fsMountedVolumeList;
                while (tmpMntVolume) {
                    struct FsVolume *tmpVolume = tmpMntVolume->mountedVolume;
                    read += sprintf(buffer + read, "%s,%u,%u\n",
                                    tmpVolume->fs->name, tmpVolume->refcount, tmpVolume->blockSize
                    );
                    tmpMntVolume = tmpMntVolume->next;
                }
                break;
            case -2: {
                u32 total, used;
                kmallocGetInfo(&total, &used);
                read = sprintf(buffer, "[PHYSMEM]\nused:%u\ntotal:%u\n\n[KERNEL MEM]\nused:%u\npaged:%u\n",
                               getTotalUsedPhysMemory(), getTotalPhysMemory(), total, used);
                break;
            }
            case -3:
                read = getCurrentDateAndTime(buffer);
                buffer[read++] = '\n';
                buffer[read] = '\0';
                break;
            case -4:
                read = sprintf(buffer, "%lu\n", gettick());
                break;
            case -5:
                read = sprintf(buffer, "pid:%u\ngid:%u\ncmdline:%s\nprivilege:%s\nevent:%d,%lu,%u\n",
                               currentTask->pid,
                               (currentTask->parent ? currentTask->parent->pid : 0),
                               (currentTask->cmdline ? currentTask->cmdline : "NONE"),
                               (currentTask->privilege == TaskPrivilegeKernel ? "KERNEL" : "USER"),
                               currentTask->event.type, currentTask->event.timer, currentTask->event.arg
                );
                break;
            default:
                read = -1;
        }
    } else {
        struct Task *task = getTaskByPid((pid_t) procPath->data);
        if (!task)
            return -1;

        read = sprintf(buffer, "pid:%u\ncmdline:%s\nprivilege:%s\nevent:%d,%lu,%u\n",
                       task->pid,
                       (task->cmdline ? task->cmdline : "NONE"),
                       (task->privilege == TaskPrivilegeKernel ? "KERNEL" : "USER"),
                       task->event.type, task->event.timer, task->event.arg
        );
    }

    return read;
}

static struct Fs fs_procfs = {
        .name = "procfs",
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