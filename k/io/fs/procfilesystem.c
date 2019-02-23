//
// Created by rebut_p on 21/12/18.
//

#include <io/fs/filesystem.h>
#include <system/allocator.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <stdlib.h>
#include <include/kstdio.h>
#include "procfilesystem.h"
#include <io/device/proc.h>
#include <errno-base.h>

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define STATIC_PROC_DATA_NB 8
static struct ProcPath staticProcData[] = {
        {PP_FOLDER, 0, "."},
        {PP_FOLDER, 0, ".."},
        {PP_INFO,   0, "mounts"},
        {PP_INFO,   1, "meminfo"},
        {PP_INFO,   2, "time"},
        {PP_INFO,   3, "uptime"},
        {PP_INFO,   4, "self"},
        {PP_INFO,   5, "version"}
};

static struct FsPath *procRoot(struct FsVolume *volume)
{
    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "procRoot");

    if (!rootPath)
        return NULL;

    (void) volume;
    rootPath->privateData = &(staticProcData[0]);
    rootPath->inode = 0;
    rootPath->type = FS_FOLDER;
    rootPath->mode = S_IRUSR | S_IRGRP | S_IROTH;
    return rootPath;
}

static struct FsVolume *procMount(struct FsPath *data)
{
    LOG("[proc] mount:\n");
    (void) data;

    struct FsVolume *procVolume = kmalloc(sizeof(struct FsVolume), 0, "newProcVolume");
    if (procVolume == NULL)
        return NULL;

    procVolume->blockSize = PROC_BLK_DATA_SZ;
    procVolume->privateData = NULL;
    return procVolume;
}

static int procUmount(struct FsVolume *volume)
{
    kfree(volume);
    return 0;
}

static int procClose(struct FsPath *path)
{
    struct ProcPath *tmp = path->privateData;
    if (tmp->type == PP_PROC)
        kfree(path->privateData);
    kfree(path);
    return 0;
}

static int procStat(struct FsPath *path, struct stat *result)
{
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;
    if (procPath->type == PP_FOLDER) {
        result->st_mode = S_IFDIR | S_IRWXU;
    } else {
        result->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
    }

    result->st_ino = (u32) procPath->data;
    result->st_size = 0;
    result->st_blksize = path->volume->blockSize;

    result->st_gid = result->st_uid = result->st_nlink = 0;
    result->st_atim = result->st_ctim = result->st_mtim = 0;

    return 0;
}

static struct FsPath *procLookup(struct FsPath *path, const char *name)
{
    (void) path;
    struct ProcPath *procPath = NULL;

    if (*name == 0 || strcmp(name, ".") == 0) {
        procPath = &(staticProcData[0]);
    } else {
        for (u32 i = 0; i < STATIC_PROC_DATA_NB; i++) {
            if (strcmp(name, staticProcData[i].name) == 0) {
                procPath = &(staticProcData[i]);
                break;
            }
        }

        if (procPath == NULL) {
            char *err;
            int pid = strtol(name, &err, 10);
            if (err == name || *err != '\0')
                return NULL;

            struct Task *task;
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
    }

    LOG("[proc] lookup enter\n");
    struct FsPath *file = kmalloc(sizeof(struct FsPath), 0, "FsPath");
    if (!file) {
        if (procPath->type == PP_PROC)
            kfree(procPath);
        return NULL;
    }

    file->privateData = procPath;
    file->mode = S_IRUSR | S_IRGRP | S_IROTH;
    file->size = 0;
    file->inode = 0;
    file->type = (procPath->type == PP_FOLDER ? FS_FOLDER : FS_FILE);
    return file;
}

static int procReaddir(struct FsPath *path, void *block, u32 nblock)
{
    struct ProcPath *procPath = (struct ProcPath *) path->privateData;

    if (!procPath || procPath->type != PP_FOLDER)
        return -ENOTDIR;

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
        pid++;
    }

    return size;
}

static struct Kobject *procOpenFile(struct FsPath *proc)
{
    struct ProcPath *p = proc->privateData;
    struct Kobject *obj = koCreate(KO_UNDEFINED, NULL, 0);
    if (obj == NULL)
        return NULL;

    if (p->type == PP_FOLDER) {
        obj->type = KO_FS_FOLDER;
        obj->data = proc;
        proc->refcount += 1;
    } else {
        obj->type = KO_PROC;
        obj->data = p;
    }

    return obj;
}

static struct Fs fs_procfs = {
        .name = "procfs",
        .mount = &procMount,
        .umount = &procUmount,
        .root = &procRoot,
        .lookup = &procLookup,
        .readdir = &procReaddir,
        .stat = &procStat,
        .close = &procClose,
        .openFile = &procOpenFile
};

void initProcFileSystem()
{
    fsRegister(&fs_procfs);
}