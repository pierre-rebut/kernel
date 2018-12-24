//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_FILESYSTEM_H
#define KERNEL_FILESYSTEM_H

#include <k/types.h>
typedef s32 off_t;

#define MAX_NB_FILE 255
#define MAX_NB_FOLDER 50

enum FileType {
    FT_DIRECTORY,
    FT_FILE
};

struct stat {
    u32 inumber;
    u32 file_sz;
    u32 idx;
    u32 blk_cnt;
    u32 d_blk_cnt;
    u32 i_blk_cnt;
    u32 cksum;
};

struct dirent {
    u32          d_ino;
    u32          d_off;       // décalage jusqu'à la dirent suivante
    unsigned short d_reclen;    // longueur de cet enregistrement
    enum FileType  d_type;      // type du fichier
    char           d_name[256]; // nom du fichier
};

struct FsVolume;

struct Fs {
    char *name;

    struct FsPath *(*root)(struct FsVolume *volume);
    struct FsVolume *(*mount)(void *data);
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
    char id;
    struct Fs *fs;
    u32 blockSize;
    int refcount;
    void *privateData;
    struct FsPath *root;
    struct FsVolume *next;
    struct FsVolume *prev;
};

struct FsPath {
    struct FsVolume *volume;
    u32 size;
    void *privateData;
    int refcount;
};

struct FsPath *fsResolvePath(const char *path);

void fsRegister(struct Fs *fs);
struct Fs *fsGetFileSystemByName(const char *name);

struct FsVolume *fsVolumeOpen(char id, struct Fs *fs, void *data);
struct FsVolume *fsGetVolumeById(char mountPoint);
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

extern struct FsVolume *fsVolumeList;

#endif //KERNEL_FILESYSTEM_H
