//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_FILESYSTEM_H
#define KERNEL_FILESYSTEM_H

#include <k/ktypes.h>
#include <types.h>
typedef s32 off_t;

#define MAX_NB_FILE 255
#define MAX_NB_FOLDER 50

struct FsVolume;

struct Fs {
    char *name;

    struct FsPath *(*root)(struct FsVolume *volume);
    struct FsVolume *(*mount)(u32 data);
    int (*umount)(struct FsVolume *volume);
    int (*close)(struct FsPath *path);
    int (*mkdir)(struct FsPath *path, const char *name);
    int (*mkfile)(struct FsPath *path, const char *name);
    int (*stat)(struct FsPath *path, struct stat *result);

    struct FsPath *(*lookup)(struct FsPath * path, const char *name);
    struct dirent *(*readdir)(struct FsPath *path, struct dirent *result);
    int (*rmdir)(struct FsPath *path, const char *name);
    int (*readBlock)(struct FsPath *path, char *buffer, u32 blocknum);
    int (*writeBlock)(struct FsPath *path, const char *buffer, u32 blocknum);

    struct Fs *next;
};

struct FsVolume {
    struct Fs *fs;
    u32 blockSize;
    int refcount;
    void *privateData;
    struct FsPath *root;
};

struct FsMountVolume {
    struct FsVolume *volumeId;
    u32 inodeId;

    struct FsVolume *mountedVolume;

    struct FsMountVolume *next;
    struct FsMountVolume *prev;
};

struct FsPath {
    u32 inode;

    struct FsVolume *volume;
    u32 size;
    void *privateData;
    int refcount;
};

struct FsPath *fsResolvePath(const char *path);

void fsRegister(struct Fs *fs);
struct Fs *fsGetFileSystemByName(const char *name);

struct FsMountVolume *fsMountVolumeOn(struct FsPath *mntPoint, struct Fs *fs, u32 data);
int fsUmountVolume(struct FsPath *mntPoint);

struct FsVolume *fsVolumeOpen(struct Fs *fs, u32 data);
struct FsMountVolume *fsGetMountedVolumeByNode(struct FsVolume *v, u32 inode);
struct FsPath *fsVolumeRoot(struct FsVolume *volume);
int fsVolumeClose(struct FsVolume *volume);
;
int fsReadFile(struct FsPath *file, char *buffer, u32 length, u32 offset);
int fsWriteFile(struct FsPath *file, const char *buffer, u32 length, u32 offset);

int fsStat(struct FsPath *file, struct stat *result);

struct FsPath *fsGetPathByName(struct FsPath *path, const char *name);
struct dirent *fsPathReaddir(struct FsPath *path, struct dirent *result);
int fsRmdir(struct FsPath *path, const char *name);
int fsMkdir(struct FsPath *path, const char *name);
int fsMkfile(struct FsPath *path, const char *name);
int fsPathDestroy(struct FsPath *path);

extern struct FsMountVolume *fsMountedVolumeList;
extern struct FsVolume *fsRootVolume;

#endif //KERNEL_FILESYSTEM_H
