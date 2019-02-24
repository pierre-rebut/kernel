//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_FILESYSTEM_H
#define KERNEL_FILESYSTEM_H

#include <stddef.h>
#include <ctype.h>
#include <sys/types.h>

#define MAX_NB_KOBJECT 255

enum FS_TYPE
{
    FS_FOLDER,
    FS_FILE
};

struct FsVolume;

struct Fs
{
    char *name;

    struct FsPath *(*root)(struct FsVolume *volume);

    struct FsVolume *(*mount)(struct FsPath *dev);

    int (*umount)(struct FsVolume *volume);

    int (*close)(struct FsPath *path);

    struct FsPath *(*mkfile)(struct FsPath *parentDir, const char *name, mode_t mode);

    struct FsPath *(*mkdir)(struct FsPath *parentDir, const char *name, mode_t mode);

    struct FsPath *(*link)(struct FsPath *nodeToLink, struct FsPath *parentDir, const char *name);

    int (*unlink)(struct FsPath *parentDir, struct FsPath *path);

    int (*chmod)(struct FsPath *path, mode_t mode);

    int (*stat)(struct FsPath *path, struct stat *result);

    struct FsPath *(*lookup)(struct FsPath *path, const char *name);

    int (*readdir)(struct FsPath *path, void *block, u32 nblock);

    int (*readBlock)(struct FsPath *path, char *buffer, u32 blocknum);

    int (*writeBlock)(struct FsPath *path, const char *buffer, u32 blocknum);

    int (*resizeFile)(struct FsPath *path, u32 newSize);

    struct Kobject *(*openFile)(struct FsPath *path);

    struct Fs *next;
};

struct FsVolume
{
    struct Fs *fs;
    u32 blockSize;
    int refcount;
    void *privateData;
    struct FsPath *root;
};

struct FsMountVolume
{
    struct FsVolume *volumeId;
    u32 inodeId;

    struct FsVolume *mountedVolume;

    struct FsMountVolume *next;
    struct FsMountVolume *prev;
};

struct FsPath
{
    u32 inode;
    u16 mode;
    enum FS_TYPE type;

    struct FsVolume *volume;
    u32 size;
    void *privateData;
    int refcount;
};

struct FsPath *fsResolvePath(const char *path);

struct FsPath *fsResolvePath2(const char *path);

void fsRegister(struct Fs *fs);

struct Fs *fsGetFileSystemByName(const char *name);

struct FsMountVolume *fsMountVolumeOn(struct FsPath *mntPoint, struct Fs *fs, struct FsPath *devicePath);

int fsUmountVolume(struct FsPath *mntPoint);

struct FsVolume *fsVolumeOpen(struct Fs *fs, struct FsPath *dev);

struct FsMountVolume *fsGetMountedVolumeByNode(struct FsVolume *v, u32 inode);

struct FsMountVolume *fsGetMountedVolume(struct FsVolume *v);

struct FsPath *fsGetParentDir(struct FsPath *path);

struct FsPath *fsVolumeRoot(struct FsVolume *volume);

int fsVolumeClose(struct FsVolume *volume);

int fsReadFile(struct FsPath *file, char *buffer, u32 length, u32 offset);

int fsWriteFile(struct FsPath *file, const char *buffer, u32 length, u32 offset);

int fsStat(struct FsPath *file, struct stat *result);

struct FsPath *fsGetPathByName(struct FsPath *path, const char *name);

int fsPathReaddir(struct FsPath *path, void *block, u32 nblock);

struct FsPath *fsMkDir(const char *name, mode_t mode);

struct FsPath *fsMkFile(const char *name, mode_t mode);

struct FsPath *fsLink(const char *name, const char *linkTo);

int fsUnlink(const char *name);

int fsChmod(struct FsPath *path, mode_t mode);

int fsPathDestroy(struct FsPath *path);

int fsResizeFile(struct FsPath *path, u32 s);

struct Kobject *fsOpenFile(struct FsPath *path, int mode);

extern struct FsMountVolume *fsMountedVolumeList;
extern struct FsVolume *fsRootVolume;

#endif //KERNEL_FILESYSTEM_H
