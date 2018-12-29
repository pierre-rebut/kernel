//
// Created by rebut_p on 17/12/18.
//

#include <string.h>
#include <sys/allocator.h>
#include <sys/physical-memory.h>
#include <task.h>
#include <include/stdio.h>

static struct Fs *fsList = NULL;
struct FsVolume *fsVolumeList = NULL;

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
#define LOG(x, ...)

struct FsPath *fsResolvePath(const char *path) {
    if (path[0] == '/') {
        LOG("[FS] resolve path 1\n");
        return fsGetPathByName(currentTask->currentDir->volume->root, &path[1]);
    }
    if (strlen(path) < 3 || path[1] != ':') {
        LOG("[FS] resolve path 2\n");
        return fsGetPathByName(currentTask->currentDir, path);
    }

    LOG("[FS] resolve path 3\n");
    struct FsVolume *volume = fsGetVolumeById(path[0]);
    if (volume == NULL)
        return NULL;
    LOG("[FS] volume found: %c\n", volume->id);
    return fsGetPathByName(volume->root, &path[3]);
}

struct FsVolume *fsGetVolumeById(char mountPoint) {
    struct FsVolume *tmpVolume = fsVolumeList;
    while (tmpVolume != NULL) {
        if (tmpVolume->id == mountPoint)
            return tmpVolume;
        tmpVolume = tmpVolume->next;
    }
    return NULL;
}

void fsRegister(struct Fs *f) {
    f->next = fsList;
    fsList = f;
}

struct Fs *fsGetFileSystemByName(const char *name) {
    struct Fs *fs = fsList;

    while (fs != NULL) {
        if (!strcmp(name, fs->name))
            return fs;
        fs = fs->next;
    }
    return NULL;
}

struct FsVolume *fsVolumeOpen(char id, struct Fs *fs, u32 data) {
    LOG("[FS] check if %c is used\n", id);
    if (!fs || !fs->mount)
        return NULL;

    LOG("[FS] utils with fs fct\n");
    struct FsVolume *volume = fs->mount(data);
    if (volume == NULL)
        return NULL;

    LOG("[FS] init volume\n");

    volume->id = id;
    volume->fs = fs;
    volume->refcount = 0;

    volume->root = fsVolumeRoot(volume);
    if (volume->root == NULL) {
        kfree(volume);
        return NULL;
    }

    if (fsVolumeList == NULL) {
        fsVolumeList = volume;
        volume->prev = NULL;
    }
    else {
        struct FsVolume *tmpVolume = fsVolumeList;
        while (tmpVolume->next != NULL)
            tmpVolume = tmpVolume->next;
        tmpVolume->next = volume;
        volume->prev = tmpVolume;
    }

    volume->next = NULL;
    return volume;
}

int fsVolumeClose(struct FsVolume *v) {
    if (!v || !v->fs->umount || v->refcount > 1)
        return -1;

    if (v->next)
        v->next->prev = v->prev;
    if (v->prev)
        v->prev->next = v->next;
    else
        fsVolumeList = v->next;

    return v->fs->umount(v);
}

struct FsPath *fsVolumeRoot(struct FsVolume *volume) {
    if (!volume || !volume->fs->root)
        return NULL;

    struct FsPath *pathRoot = volume->fs->root(volume);
    if (pathRoot == NULL)
        return NULL;

    pathRoot->volume = volume;
    volume->refcount++;
    pathRoot->refcount = 1;
    return pathRoot;
}

struct dirent *fsPathReaddir(struct FsPath *path, struct dirent *result) {
    if (!path || !path->volume->fs->readdir)
        return NULL;
    LOG("readdir\n");
    return path->volume->fs->readdir(path, result);
}

static struct FsPath *fsPathLookup(struct FsPath *path, const char *name) {
    if (!path || !path->volume->fs->lookup)
        return NULL;

    LOG("[FS] lookup fct\n");
    struct FsPath *newPath = path->volume->fs->lookup(path, name);
    if (!newPath)
        return NULL;

    LOG("[FS] lookup path not null\n");

    newPath->volume = path->volume;
    newPath->volume->refcount++;
    newPath->refcount = 1;
    return newPath;
}

// todo fix bug
struct FsPath *fsGetPathByName(struct FsPath *d, const char *path) {
    if (!d || !path)
        return NULL;

    if (*path == 0) {
        return fsPathLookup(d, path);
    }

    LOG("[FS] kmalloc %s\n", path);
    char *lpath = kmalloc(strlen(path) + 1, 0, "tmpPath");
    strcpy(lpath, path);

    d->refcount++;
    LOG("[FS] ref count: %u\n", d->refcount);
    struct FsPath *new = d;
    struct FsPath *new2 = NULL;

    char *part = strtok(lpath, "/");
    LOG("[FS] strtok\n");
    while (part) {
        LOG("[FS] lookup\n");
        new2 = fsPathLookup(new, part);
        LOG("[FS] destroy old\n");
        fsPathDestroy(new);
        LOG("[FS] check\n");
        if (!new2)
            break;

        part = strtok(0, "/");
        new = new2;
    }
    LOG("[FS] end\n");
    kfree(lpath);
    return new2;
}

int fsPathDestroy(struct FsPath *path) {
    if (!path || !path->volume->fs->close)
        return -1;

    path->refcount--;
    if (path->refcount > 0)
        return 0;

    path->volume->fs->close(path);
    path->volume->refcount--;
    return 0;
}

int fsReadFile(struct FsPath *file, char *buffer, u32 length, u32 offset) {
    int total = 0;
    u32 bs = file->volume->blockSize;

    if (!file->volume->fs->readBlock)
        return -1;

    char *temp = kmalloc(bs, 0, "fsFileRead");
    if (!temp)
        return -1;

    u32 blocknum = offset / bs;;
    int readSize;
    int actual = 0;

    LOG("[FS] read start (offset: %u, blocknum: %u, length: %u)\n", offset, blocknum, length);

    do {

        readSize = file->volume->fs->readBlock(file, temp, blocknum);
        if (readSize <= 0)
            goto failure;

        if (offset % bs) {
            LOG("[FS] offset % bs: %d\n", readSize - offset % bs);
            actual = MIN(readSize - offset % bs, (u32) readSize);
            actual = MIN(actual, (int) length);

            if (actual > 0)
                memcpy(buffer, &temp[offset % bs], (u32) actual);
        } else if (length >= bs) {
            LOG("[FS] length >= bs\n");
            memcpy(buffer, temp, (u32) readSize);
            actual = readSize;
        } else {
            LOG("[FS] length < bs\n");
            actual = MIN(readSize, (int) length);
            memcpy(buffer, temp, (u32) actual);
        }

        LOG("[FS] actual = %d\n", actual);

        buffer += actual;
        length -= actual;
        offset += actual;
        total += actual;
        blocknum++;

    } while (length > 0 && actual && readSize == (int) bs);

    kfree(temp);
    LOG("[FS] read end: %d\n", total);
    return total;

    failure:
    LOG("[FS] read failure: %d\n", readSize);
    kfree(temp);
    if (total == 0)
        return -1;
    return total;
}

int fsMkdir(struct FsPath *path, const char *name) {
    if (!path || !name || !path->volume->fs->mkdir)
        return 0;
    return path->volume->fs->mkdir(path, name);
}

int fsMkfile(struct FsPath *path, const char *name) {
    if (!path || !name || !path->volume->fs->mkfile)
        return 0;
    return path->volume->fs->mkfile(path, name);
}

int fsRmdir(struct FsPath *path, const char *name) {
    if (!path || !name || !path->volume->fs->rmdir)
        return 0;
    return path->volume->fs->rmdir(path, name);
}

int fsWriteFile(struct FsPath *file, const char *buffer, u32 length, u32 offset) {
    int total = 0;
    u32 bs = file->volume->blockSize;

    if (!file->volume->fs->writeBlock || !file->volume->fs->readBlock)
        return -1;

    char *temp = kmalloc(4096, PAGESIZE, "fsFileWrite");

    while (length > 0) {

        int blocknum = offset / bs;
        int actual = 0;

        if (offset % bs) {
            actual = file->volume->fs->readBlock(file, temp, blocknum);
            if (actual != (int) bs)
                goto failure;

            actual = MIN(bs - offset % bs, length);
            memcpy(&temp[offset % bs], buffer, (u32) actual);

            int wactual = file->volume->fs->writeBlock(file, temp, blocknum);
            if (wactual != (int) bs)
                goto failure;

        } else if (length >= bs) {
            actual = file->volume->fs->writeBlock(file, buffer, blocknum);
            if (actual != (int) bs)
                goto failure;
        } else {
            actual = file->volume->fs->readBlock(file, temp, blocknum);
            if (actual != (int) bs)
                goto failure;

            actual = length;
            memcpy(temp, buffer, (u32) actual);

            int wactual = file->volume->fs->writeBlock(file, temp, blocknum);
            if (wactual != (int) bs)
                goto failure;
        }

        buffer += actual;
        length -= actual;
        offset += actual;
        total += actual;
    }

    kfree(temp);
    return total;

    failure:
    kfree(temp);
    if (total == 0)
        return -1;
    return total;
}

int fsStat(struct FsPath *path, struct stat *result) {
    if (!path || !path->volume->fs->stat)
        return -1;

    return path->volume->fs->stat(path, result);
}
