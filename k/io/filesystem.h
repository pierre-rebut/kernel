//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_FILESYSTEM_H
#define KERNEL_FILESYSTEM_H

#include <k/types.h>
typedef s32 off_t;

#define MAX_NB_FILE 255

struct FileDescriptor {
    void *entryData;

    s32 (*readFct)(void *, void *buf, u32 size);
    s32 (*writeFct)(void *, void *buf, u32 size);
    off_t (*seekFct)(void *, off_t offset, int whence);
    int (*closeFct)(void*);
};

int open(const char *pathname, int flags);
s32 read(int fd, void *buf, u32 size);
s32 write(int fd, void *buf, u32 size);
off_t seek(int fd, off_t offset, int whence);
int close(int fd);

struct FileDescriptor *createFileDescriptor(s32 (*readFct)(void *, void *, u32),
                                            s32 (*writeFct)(void *, void *, u32),
                                            off_t (*seekFct)(void *, off_t, int),
                                            int (*closeFct)(void *));

#endif //KERNEL_FILESYSTEM_H
