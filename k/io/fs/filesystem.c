//
// Created by rebut_p on 17/12/18.
//

#include <string.h>
#include <sys/allocator.h>
#include <task.h>
#include <include/kstdio.h>

static struct Fs *fsList = NULL;
struct FsMountVolume *fsMountedVolumeList = NULL;
struct FsVolume *fsRootVolume = NULL;

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

struct FsPath *fsResolvePath(const char *path) {
    if (path[0] == '/') {
        LOG("[FS] resolve path 1\n");
        return fsGetPathByName(currentTask->rootDir, path + 1);
    }

    LOG("[FS] resolve path 2\n");
    return fsGetPathByName(currentTask->currentDir, path);
}

struct FsMountVolume *fsGetMountedVolumeByNode(struct FsVolume *v, u32 inode) {
    struct FsMountVolume *tmpVolume = fsMountedVolumeList;
    while (tmpVolume != NULL) {
        if (tmpVolume->volumeId == v && tmpVolume->inodeId == inode)
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

struct FsVolume *fsVolumeOpen(struct Fs *fs, u32 data) {
    LOG("[FS] utils with fs fct\n");
    struct FsVolume *volume = fs->mount(data);
    if (volume == NULL)
        return NULL;

    LOG("[FS] init volume\n");
    volume->fs = fs;
    volume->refcount = 0;

    volume->root = fsVolumeRoot(volume);
    if (volume->root == NULL) {
        kfree(volume);
        return NULL;
    }

    return volume;
}

struct FsMountVolume *fsMountVolumeOn(struct FsPath *mntPoint, struct Fs *fs, u32 data) {
    if (mntPoint->inode == 0 || fsGetMountedVolumeByNode(mntPoint->volume, mntPoint->inode) != NULL)
        return NULL;

    struct FsMountVolume *mntVolume = kmalloc(sizeof(struct FsMountVolume), 0, "FsMountVol");
    if (mntVolume == NULL)
        return NULL;

    mntVolume->mountedVolume = fsVolumeOpen(fs, data);
    if (mntVolume->mountedVolume == NULL) {
        kfree(mntVolume);
        return NULL;
    }

    mntVolume->volumeId = mntPoint->volume;
    mntVolume->volumeId->refcount += 1;
    mntVolume->inodeId = mntPoint->inode;

    mntVolume->prev = NULL;
    mntVolume->next = fsMountedVolumeList;
    fsMountedVolumeList = mntVolume;

    return mntVolume;
}

int fsVolumeClose(struct FsVolume *v) {
    if (!v || !v->fs->umount || v->refcount > 1)
        return -1;

    return v->fs->umount(v);
}

int fsUmountVolume(struct FsPath *mntPoint) {
    struct FsMountVolume *mntVolume = fsGetMountedVolumeByNode(mntPoint->volume, mntPoint->inode);
    if (mntVolume == NULL)
        return 0;

    if (fsVolumeClose(mntVolume->mountedVolume) == -1)
        return -1;

    mntVolume->volumeId->refcount -= 1;
    if (mntVolume->prev)
        mntVolume->prev->next = mntVolume->next;
    else
        fsMountedVolumeList = mntVolume->next;

    if (mntVolume->next)
        mntVolume->next->prev = mntVolume->prev;
    kfree(mntVolume);
    return 0;
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
    if (!path)
        return NULL;

    struct FsPath *curPath = path;
    struct FsMountVolume *mntVol = fsGetMountedVolumeByNode(curPath->volume, curPath->inode);
    if (mntVol != NULL)
        curPath = mntVol->mountedVolume->root;

    if (!curPath->volume->fs->lookup)
        return NULL;

    if (strcmp(name, "..") == 0) {
        curPath = path;
    }

    LOG("[FS] lookup fct\n");
    struct FsPath *newPath = curPath->volume->fs->lookup(curPath, name);
    if (!newPath)
        return NULL;

    LOG("[FS] lookup path not null\n");
    newPath->volume = curPath->volume;
    newPath->volume->refcount++;
    newPath->refcount = 1;
    return newPath;
}

// todo fix bug
struct FsPath *fsGetPathByName(struct FsPath *d, const char *path) {
    if (!d || !path)
        return NULL;

    if (*path == 0) {
        return fsPathLookup(d, ".");
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

    char *temp = kmalloc(4096, 0, "fsFileWrite");
    if (temp == NULL)
        return -1;

    klog("test 1: %u\n", length);
    if (offset + length > file->size) {
        if (file->volume->fs->resizeFile == NULL)
            return -1;
        file->volume->fs->resizeFile(file, offset + length);
    }

    while (length > 0) {

        int blocknum = offset / bs;
        int actual = 0;

        if (offset % bs) {
            actual = file->volume->fs->readBlock(file, temp, blocknum);
            if (actual <= 0)
                goto failure;

            actual = MIN(actual - offset % bs, (u32) actual);
            actual = MIN(actual, (int) length);

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
            if (actual <= 0)
                goto failure;

            actual = MIN(actual, (int) length);
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
    if (result == NULL)
        return 0;

    if (!path || !path->volume->fs->stat)
        return -1;

    return path->volume->fs->stat(path, result);
}
