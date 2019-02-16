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
#include <include/kstdio.h>
#include "procfilesystem.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

enum ProcPathType {
    PP_INFO,
    PP_PROC,
    PP_FOLDER
};

struct ProcPath {
    enum ProcPathType type;
    int data;
    char *name;
};

#define STATIC_PROC_DATA_NB 7
static struct ProcPath staticProcData[] = {
        {PP_FOLDER, 0, "."},
        {PP_INFO, 0, "mounts"},
        {PP_INFO, 1, "meminfo"},
        {PP_INFO, 2, "time"},
        {PP_INFO, 3, "uptime"},
        {PP_INFO, 4, "self"},
        {PP_INFO, 5, "version"}
};

static struct FsPath *procRoot(struct FsVolume *volume) {
    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "procRoot");

    if (!rootPath)
        return NULL;

    (void) volume;
    rootPath->privateData = &(staticProcData[0]);
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
    struct ProcPath *tmp = path->privateData;
    if (tmp->type == PP_PROC)
        kfree(path->privateData);
    kfree(path);
    return 0;
}

static int procStat(struct FsPath *path, struct stat *result) {
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;
    if (procPath->type == PP_FOLDER)
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

    struct ProcPath *procPath = NULL;
    for (u32 i = 0; i < STATIC_PROC_DATA_NB; i++) {
        if (strcmp(name, staticProcData[i].name) == 0) {
            procPath = &(staticProcData[i]);
            break;
        }
    }

    if (procPath == NULL) {
        struct Task *task;
        int pid = atoi(name);
        if (pid == 0)
            task = &kernelTask;
        else
            task = getTaskByPid(pid);

        if (task == NULL)
            return NULL;

        procPath = kmalloc(sizeof(struct ProcPath), 0, "procPath");
        if (procPath == NULL)
            return NULL;

        procPath->type = PP_PROC;
        procPath->data = task->pid;
    }

    LOG("[proc] lookup enter\n");
    struct FsPath *file = kmalloc(sizeof(struct FsPath), 0, "FsPath");
    if (!file) {
        if (procPath->type == PP_PROC)
            kfree(procPath);
        return NULL;
    }

    file->privateData = procPath;
    file->size = 0;
    file->inode = 0;
    return file;
}

u32 procReaddir(struct FsPath *path, void *block, u32 nblock) {
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;

    if (!procPath || procPath->type != PP_FOLDER)
        return 0;

    u32 size = 0;
    u32 i = nblock * DIRENT_BUFFER_NB;
    klog("[proc] readdir: %u - %u\n", nblock, i);

    while (size < DIRENT_BUFFER_NB && i < STATIC_PROC_DATA_NB) {
        struct dirent tmpDirent;
        strcpy(tmpDirent.d_name, staticProcData[i].name);
        tmpDirent.d_namlen = strlen(tmpDirent.d_name);
        tmpDirent.d_type = FT_FILE;
        tmpDirent.d_ino = i;

        memcpy(block, &tmpDirent, sizeof(struct dirent));
        block += sizeof(struct dirent);

        i++;
        size++;
    }

    pid_t pid = i - STATIC_PROC_DATA_NB;
    while (size < DIRENT_BUFFER_NB && pid < TASK_MAX_PID) {
        struct Task *task = getTaskByPid(pid);
        if (task) {
            struct dirent tmpDirent;
            tmpDirent.d_namlen = (u32) sprintf(tmpDirent.d_name, "%u", pid);
            tmpDirent.d_type = FT_FILE;
            tmpDirent.d_ino = i;

            memcpy(block, &tmpDirent, sizeof(struct dirent));
            block += sizeof(struct dirent);

            size += 1;
        }
        pid ++;
    }

    return size;
}

static int procReadBlock(struct FsPath *path, char *buffer, u32 blocknum) {
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;
    LOG("[proc] readblock: %u\n", blocknum);
    if (!procPath || blocknum > 0)
        return -1;

    switch (procPath->type) {
        case PP_PROC: {
            struct Task *task = getTaskByPid((pid_t) procPath->data);
            if (!task)
                return -1;

            return sprintf(buffer, "pid:%u\ncmdline:%s\nprivilege:%s\nevent:%d,%lu,%u\n",
                           task->pid,
                           (task->cmdline ? task->cmdline : "NONE"),
                           (task->privilege == TaskPrivilegeKernel ? "KERNEL" : "USER"),
                           task->event.type, task->event.timer, task->event.arg
            );
        }
        case PP_INFO: {
            int read;
            switch (procPath->data) {
                case 0:
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
                case 1: {
                    u32 total, used;
                    kmallocGetInfo(&total, &used);
                    read = sprintf(buffer, "[PHYSMEM]\nused:%u\ntotal:%u\n\n[KERNEL MEM]\nused:%u\npaged:%u\n",
                                   getTotalUsedPhysMemory(), getTotalPhysMemory(), total, used);
                    break;
                }
                case 2:
                    read = getCurrentDateAndTime(buffer);
                    buffer[read++] = '\n';
                    buffer[read] = '\0';
                    break;
                case 3:
                    read = sprintf(buffer, "%lu\n", gettick());
                    break;
                case 4:
                    read = sprintf(buffer, "pid:%u\ngid:%u\ncmdline:%s\nprivilege:%s\nevent:%d,%lu,%u\n",
                                   currentTask->pid,
                                   (currentTask->parent ? currentTask->parent->pid : 0),
                                   (currentTask->cmdline ? currentTask->cmdline : "NONE"),
                                   (currentTask->privilege == TaskPrivilegeKernel ? "KERNEL" : "USER"),
                                   currentTask->event.type, currentTask->event.timer, currentTask->event.arg
                    );
                    break;
                case 5:
                    read = sprintf(buffer, "K-OS version 1.0-arch\n");
                    break;
                default:
                    return -1;
            }
            return read;
        }
        default:
            return -1;
    }
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