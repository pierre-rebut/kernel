//
// Created by rebut_p on 24/12/18.
//

#include <sys/allocator.h>
#include <kstdio.h>
#include <string.h>
#include <io/device/device.h>

#include "devfilesystem.h"
#include "filesystem.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static struct FsPath *devRoot(struct FsVolume *volume) {
    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "devRoot");
    if (!rootPath)
        return NULL;

    (void) volume;

    rootPath->size = 0;
    rootPath->privateData = 0;
    rootPath->inode = 0;
    return rootPath;
}

static struct FsVolume *devMount(u32 data) {
    LOG("[dev] mount:\n");
    (void) data;

    struct FsVolume *devVolume = kmalloc(sizeof(struct FsVolume), 0, "newDevVolume");
    if (devVolume == NULL)
        return NULL;

    devVolume->blockSize = DEV_BLK_DATA_SZ;
    devVolume->privateData = 0;
    return devVolume;
}

static int devUmount(struct FsVolume *volume) {
    kfree(volume);
    return 0;
}

static int devClose(struct FsPath *path) {
    kfree(path);
    return 0;
}

static struct FsPath *devLookup(struct FsPath *path, const char *name) {
    (void) path;

    if (*name != 0 && strcmp(name, ".") != 0)
        return NULL;

    LOG("[dev] lookup enter\n");
    struct FsPath *file = kmalloc(sizeof(struct FsPath), 0, "FsPath");
    if (!file)
        return NULL;

    file->privateData = 0;
    file->size = 0;
    file->inode = 0;
    return file;
}

static struct dirent *devReaddir(struct FsPath *path, struct dirent *result) {
    LOG("[dev] readdir\n");
    struct DeviceDriver *driver = deviceGetDeviceDriverByIndex((u32) path->privateData);
    if (driver == NULL)
        return NULL;

    sprintf(result->d_name, "%s", driver->name);
    result->d_ino = (u32) path->privateData;
    result->d_type = FT_FILE;
    result->d_namlen = strlen(driver->name);

    path->privateData += 1;
    return result;
}

static struct Fs fs_devfs = {
        .name = "devfs",
        .mount = &devMount,
        .umount = &devUmount,
        .root = &devRoot,
        .lookup = &devLookup,
        .readdir = &devReaddir,
        .close = &devClose
};

void initDevFileSystem() {
    fsRegister(&fs_devfs);
}