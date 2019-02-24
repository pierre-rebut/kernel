//
// Created by rebut_p on 17/12/18.
//

#include <kstdio.h>
#include <errno-base.h>
#include <string.h>

#include <system/allocator.h>
#include <task.h>

static struct Fs *fsList = NULL;
struct FsMountVolume *fsMountedVolumeList = NULL;
struct FsVolume *fsRootVolume = NULL;

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

struct FsPath *fsResolvePath(const char *path)
{
    struct FsPath *starting = currentTask->currentDir;
    if (path[0] == '/') {
        path++;
        starting = currentTask->rootDir;
    }

    LOG("[FS] resolve path\n");
    struct FsPath *resPath = fsGetPathByName(starting, path);
    if (resPath == NULL)
        return NULL;

    struct FsMountVolume *mntVol = fsGetMountedVolumeByNode(resPath->volume, resPath->inode);
    if (mntVol != NULL) {
        fsPathDestroy(resPath);
        resPath = mntVol->mountedVolume->root;
        resPath->refcount += 1;
    }
    return resPath;
}

struct FsPath *fsResolvePath2(const char *path)
{
    struct FsPath *starting = currentTask->currentDir;
    if (path[0] == '/') {
        path++;
        starting = currentTask->rootDir;
    }

    LOG("[FS] resolve path 2\n");
    return fsGetPathByName(starting, path);
}

struct FsMountVolume *fsGetMountedVolumeByNode(struct FsVolume *v, u32 inode)
{
    struct FsMountVolume *tmpVolume = fsMountedVolumeList;
    while (tmpVolume != NULL) {
        if (tmpVolume->volumeId == v && tmpVolume->inodeId == inode)
            return tmpVolume;
        tmpVolume = tmpVolume->next;
    }
    return NULL;
}

struct FsMountVolume *fsGetMountedVolume(struct FsVolume *v)
{
    struct FsMountVolume *tmpVolume = fsMountedVolumeList;
    while (tmpVolume != NULL) {
        if (tmpVolume->mountedVolume == v)
            return tmpVolume;
        tmpVolume = tmpVolume->next;
    }
    return NULL;
}

void fsRegister(struct Fs *f)
{
    f->next = fsList;
    fsList = f;
}

struct Fs *fsGetFileSystemByName(const char *name)
{
    struct Fs *fs = fsList;

    while (fs != NULL) {
        if (!strcmp(name, fs->name))
            return fs;
        fs = fs->next;
    }
    return NULL;
}

struct FsVolume *fsVolumeOpen(struct Fs *fs, struct FsPath *dev)
{
    LOG("[FS] utils with fs fct\n");
    struct FsVolume *volume = fs->mount(dev);
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

struct Kobject *fsOpenFile(struct FsPath *path, int mode)
{
    if (!path || !path->volume->fs->openFile)
        return NULL;

    int tmpMode = ((mode & 0x3) << 1) << 6;
    if ((path->mode & tmpMode) != tmpMode)
        return NULL;

    struct Kobject *obj = path->volume->fs->openFile(path);
    if (obj == NULL)
        return NULL;

    obj->mode = mode & 0x3;
    return obj;
}

struct FsMountVolume *fsMountVolumeOn(struct FsPath *mntPoint, struct Fs *fs, struct FsPath *devicePath)
{
    if (mntPoint->inode == 0 || fsGetMountedVolumeByNode(mntPoint->volume, mntPoint->inode) != NULL)
        return NULL;

    struct FsMountVolume *mntVolume = kmalloc(sizeof(struct FsMountVolume), 0, "FsMountVol");
    if (mntVolume == NULL)
        return NULL;

    mntVolume->mountedVolume = fsVolumeOpen(fs, devicePath);
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

int fsVolumeClose(struct FsVolume *v)
{
    if (!v || !v->fs->umount || v->refcount > 1)
        return -EPERM;

    return v->fs->umount(v);
}

int fsUmountVolume(struct FsPath *mntPoint)
{
    struct FsMountVolume *mntVolume = fsGetMountedVolumeByNode(mntPoint->volume, mntPoint->inode);
    if (mntVolume == NULL)
        return 0;

    if (fsVolumeClose(mntVolume->mountedVolume) == -1)
        return -EIO;

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

struct FsPath *fsVolumeRoot(struct FsVolume *volume)
{
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

int fsPathReaddir(struct FsPath *path, void *block, u32 nblock)
{
    if (!block || !path->volume->fs->readdir)
        return -EPERM;

    LOG("readdir: %u\n", nblock);
    return path->volume->fs->readdir(path, block, nblock);
}

static struct FsPath *fsPathLookup(struct FsPath *path, const char *name)
{
    if (!path)
        return NULL;

    struct FsPath *curPath = path;
    struct FsMountVolume *mntVol = fsGetMountedVolumeByNode(curPath->volume, curPath->inode);
    if (mntVol != NULL)
        curPath = mntVol->mountedVolume->root;

    if (!curPath->volume->fs->lookup)
        return NULL;

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

struct FsPath *fsGetParentDir(struct FsPath *path)
{
    struct FsPath tmpPath;

    struct FsMountVolume *vol = fsGetMountedVolume(path->volume);
    if (vol != NULL) {
        tmpPath.volume = vol->volumeId;
        tmpPath.inode = vol->inodeId;
    } else {
        tmpPath.volume = path->volume;
        tmpPath.inode = path->inode;
    }

    if (!tmpPath.volume->fs->lookup)
        return NULL;

    struct FsPath *parent = tmpPath.volume->fs->lookup(&tmpPath, "..");
    if (parent == NULL)
        return NULL;

    LOG("[FS] getParentDir path not null\n");
    parent->volume = tmpPath.volume;
    parent->volume->refcount++;
    parent->refcount = 1;
    return parent;
}

struct FsPath *fsGetPathByName(struct FsPath *d, const char *path)
{
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
        if (strcmp(part, "..") == 0)
            new2 = fsGetParentDir(new);
        else
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

int fsPathDestroy(struct FsPath *path)
{
    if (!path || !path->volume->fs->close)
        return -EPERM;

    path->refcount--;
    if (path->refcount > 0)
        return 0;

    path->volume->fs->close(path);
    path->volume->refcount--;
    return 0;
}

int fsReadFile(struct FsPath *file, char *buffer, u32 length, u32 offset)
{
    int total = 0;
    u32 bs = file->volume->blockSize;

    if (!file->volume->fs->readBlock)
        return -EPERM;

    char *temp = kmalloc(bs, 0, "fsFileRead");
    if (!temp)
        return -ENOMEM;

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
        return -EIO;
    return total;
}

static char *fsSplitPath(const char *name, struct FsPath **parent)
{
    if (!name)
        return NULL;

    u32 nameSize = strlen(name) + 1;
    char *f = kmalloc(nameSize, 0, "mkfilename");
    if (f == NULL)
        return NULL;

    memcpy(f, name, nameSize);
    char *file = NULL;
    int ret = str_backspace(f, '/', &file);

    if (strcontain(file, '/') == 1) {
        kfree(f);
        return NULL;
    }

    if (ret)
        *parent = fsResolvePath(f);
    else {
        *parent = currentTask->currentDir;
        (*parent)->refcount += 1;
    }

    char *res = strdup(file);
    kfree(f);
    return res;
}

struct FsPath *fsMkFile(const char *name, mode_t mode)
{
    struct FsPath *parent;

    char *filename = fsSplitPath(name, &parent);
    if (filename == NULL || parent == NULL || parent->volume->fs->mkfile == NULL) {
        kfree(filename);
        fsPathDestroy(parent);
        return NULL;
    }

    if (strlen(filename) >= MAXPATHLEN) {
        kfree(filename);
        fsPathDestroy(parent);
        return NULL;
    }

    struct FsPath *path = parent->volume->fs->mkfile(parent, filename, mode & 0xFFF);
    kfree(filename);

    if (path == NULL) {
        fsPathDestroy(parent);
        return NULL;
    }

    path->volume = parent->volume;
    fsPathDestroy(parent);
    path->refcount = 1;
    return path;
}

struct FsPath *fsMkDir(const char *name, mode_t mode)
{
    struct FsPath *parent;

    char *filename = fsSplitPath(name, &parent);
    if (filename == NULL || parent == NULL || parent->volume->fs->mkdir == NULL) {
        kfree(filename);
        fsPathDestroy(parent);
        return NULL;
    }

    if (strlen(filename) >= MAXPATHLEN) {
        kfree(filename);
        fsPathDestroy(parent);
        return NULL;
    }

    struct FsPath *path = parent->volume->fs->mkdir(parent, filename, mode & 0xFFF);
    kfree(filename);

    if (path == NULL) {
        fsPathDestroy(parent);
        return NULL;
    }

    path->volume = parent->volume;
    fsPathDestroy(parent);
    path->refcount = 1;
    return path;
}

struct FsPath *fsLink(const char *name, const char *linkTo)
{
    if (linkTo == NULL)
        return NULL;

    struct FsPath *pathLinkTo = fsResolvePath(linkTo);
    if (pathLinkTo == NULL)
        return NULL;

    struct FsPath *parent;
    char *filename = fsSplitPath(name, &parent);
    if (filename == NULL || parent == NULL || !parent->volume->fs->link)
        goto failure;

    if (pathLinkTo->volume != parent->volume)
        goto failure;

    if (strlen(filename) >= MAXPATHLEN)
        goto failure;

    struct FsPath *path = parent->volume->fs->link(pathLinkTo, parent, filename);
    kfree(filename);
    fsPathDestroy(pathLinkTo);

    if (path == NULL) {
        fsPathDestroy(parent);
        return NULL;
    }

    path->volume = parent->volume;
    fsPathDestroy(parent);
    path->size = 0;
    path->refcount = 1;
    return path;

    failure:
    klog("[fs] link failure\n");
    kfree(filename);
    fsPathDestroy(parent);
    fsPathDestroy(pathLinkTo);
    return NULL;
}

int fsUnlink(const char *name)
{
    struct FsPath *path = fsResolvePath(name);

    struct FsPath *parent;
    char *filename = fsSplitPath(name, &parent);
    kfree(filename);

    if (parent == NULL || path == NULL) {
        fsPathDestroy(parent);
        fsPathDestroy(path);
    }
    return 0;
}

int fsWriteFile(struct FsPath *file, const char *buffer, u32 length, u32 offset)
{
    int total = 0;
    u32 bs = file->volume->blockSize;

    if (!file->volume->fs->writeBlock || !file->volume->fs->readBlock)
        return -EPERM;

    char *temp = kmalloc(4096, 0, "fsFileWrite");
    if (temp == NULL)
        return -ENOMEM;

    if (offset + length > file->size) {
        if (file->volume->fs->resizeFile == NULL)
            return -EPERM;
        file->volume->fs->resizeFile(file, offset + length);
    }

    while (length > 0) {

        u32 blocknum = offset / bs;
        int actual = 0;

        if (offset % bs) {
            actual = file->volume->fs->readBlock(file, temp, blocknum);
            if (actual < 0)
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
            if (actual < 0)
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
    klog("[FS] writeblock failure\n");
    kfree(temp);
    if (total == 0)
        return -EIO;
    return total;
}

int fsStat(struct FsPath *path, struct stat *result)
{
    if (result == NULL)
        return 0;

    if (!path || !path->volume->fs->stat)
        return -EPERM;

    return path->volume->fs->stat(path, result);
}

int fsResizeFile(struct FsPath *path, u32 size)
{
    if (!path || !path->volume->fs->resizeFile)
        return -EPERM;

    return path->volume->fs->resizeFile(path, size);
}

int fsChmod(struct FsPath *path, mode_t mode)
{
    if (!path || !path->volume->fs->chmod)
        return -EPERM;

    return path->volume->fs->chmod(path, mode & (S_IRWXU | S_IRWXG | S_IRWXO));
}