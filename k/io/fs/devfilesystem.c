//
// Created by rebut_p on 24/12/18.
//

#include <kstdio.h>
#include <string.h>

#include <io/device/device.h>
#include <system/kobject.h>
#include <system/allocator.h>

#include "devfilesystem.h"
#include "filesystem.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static struct FsPath *devRoot(struct FsVolume *volume)
{
    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "devRoot");
    if (!rootPath)
        return NULL;

    (void) volume;

    rootPath->size = 0;
    rootPath->privateData = 0;
    rootPath->inode = 0;
    rootPath->type = FS_FOLDER;
    return rootPath;
}

static struct FsVolume *devMount(struct FsPath *dev)
{
    LOG("[dev] mount:\n");
    (void) dev;

    struct FsVolume *devVolume = kmalloc(sizeof(struct FsVolume), 0, "newDevVolume");
    if (devVolume == NULL)
        return NULL;

    devVolume->blockSize = DEV_BLK_DATA_SZ;
    devVolume->privateData = 0;
    return devVolume;
}

static int devUmount(struct FsVolume *volume)
{
    kfree(volume);
    return 0;
}

static int devClose(struct FsPath *path)
{
    kfree(path);
    return 0;
}

static struct FsPath *devLookup(struct FsPath *path, const char *name)
{
    (void) path;

    LOG("[dev] lookup enter\n");
    struct FsPath *file = kmalloc(sizeof(struct FsPath), 0, "FsPath");
    if (!file)
        return NULL;

    if (*name == 0 || strcmp(name, ".") == 0) {
        file->privateData = NULL;
        file->inode = 0;
        file->type = FS_FOLDER;
    } else {
        file->privateData = deviceGetByName(name);
        if (file->privateData == NULL) {
            kfree(file);
            return NULL;
        }

        file->inode = 1;
        file->type = FS_FILE;
    }

    file->size = 0;
    file->mode = S_IRUSR | S_IRGRP | S_IROTH;
    return file;
}

static int devReaddir(struct FsPath *path, void *block, u32 nblock)
{
    (void) path;

    LOG("[dev] readdir: %u\n", nblock);
    u32 size = 0;
    u32 i = nblock * DIRENT_BUFFER_NB;

    while (size < DIRENT_BUFFER_NB) {
        struct Device *driver = deviceGetByIndex(i);
        if (driver == NULL)
            break;

        struct dirent tmpDirent;
        strcpy(tmpDirent.d_name, driver->name);
        tmpDirent.d_namlen = strlen(tmpDirent.d_name);
        tmpDirent.d_type = FT_FILE;
        tmpDirent.d_ino = i;

        memcpy(block, &tmpDirent, sizeof(struct dirent));
        block += sizeof(struct dirent);

        i++;
        size++;
    }

    return size;
}

static int devStat(struct FsPath *path, struct stat *result)
{
    if (path->type == FS_FOLDER) {
        result->st_mode = S_IFDIR | S_IRWXU;
    } else {
        result->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
    }

    result->st_ino = path->inode;
    result->st_size = 0;
    result->st_blksize = path->volume->blockSize;

    result->st_gid = result->st_uid = result->st_nlink = 0;
    result->st_atim = result->st_ctim = result->st_mtim = 0;

    return 0;
}

static struct Kobject *devOpenFile(struct FsPath *dev)
{
    struct Kobject *obj = koCreate(KO_UNDEFINED, NULL, 0);
    if (obj == NULL)
        return NULL;

    if (dev->type == FS_FOLDER) {
        obj->type = KO_FS_FOLDER;
        obj->data = dev;
        dev->refcount += 1;
    } else {
        obj->type = KO_DEVICE;
        obj->data = dev->privateData;
    }

    return obj;
}

static struct Fs fs_devfs = {
        .name = "devfs",
        .mount = &devMount,
        .umount = &devUmount,
        .root = &devRoot,
        .lookup = &devLookup,
        .stat = &devStat,
        .readdir = &devReaddir,
        .close = &devClose,
        .openFile = &devOpenFile
};

void initDevFileSystem()
{
    fsRegister(&fs_devfs);
}