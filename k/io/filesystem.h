//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_FILESYSTEM_H
#define KERNEL_FILESYSTEM_H

#include <k/types.h>
typedef s32 off_t;

#define MAX_NB_FILE 255
#define MAX_NB_FOLDER 50

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
    u32          d_off;       /* décalage jusqu'à la dirent suivante */
    unsigned short d_reclen;    /* longueur de cet enregistrement */
    unsigned char  d_type;      /* type du fichier */
    char           d_name[256]; /* nom du fichier */
};

struct FileDescriptor {
    void *entryData;

    s32 (*readFct)(void *, void *buf, u32 size);
    s32 (*writeFct)(void *, void *buf, u32 size);
    off_t (*seekFct)(void *, off_t offset, int whence);
    int (*closeFct)(void *);
    int (*statFct)(void *, struct stat *data);
};

struct FolderDescriptor {
    void *entryData;

    struct dirent *(*readFct)(void *, struct dirent *);
    int (*closeFct)(void *);
};

int open(const char *pathname, int flags);
s32 read(int fd, void *buf, u32 size);
s32 write(int fd, void *buf, u32 size);
off_t seek(int fd, off_t offset, int whence);
int close(int fd);

int stat(const char *pathname, struct stat *data);
int fstat(int fd, struct stat *data);

int opendir(const char *pathname);
struct dirent *readdir(int fd, struct dirent *data);
int closedir(int fd);

struct FileDescriptor *createFileDescriptor(s32 (*readFct)(void *, void *, u32),
                                            s32 (*writeFct)(void *, void *, u32),
                                            off_t (*seekFct)(void *, off_t, int),
                                            int (*closeFct)(void *),
                                            int (*statFct)(void *, struct stat *));

#endif //KERNEL_FILESYSTEM_H
