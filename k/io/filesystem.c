//
// Created by rebut_p on 15/12/18.
//

#include <sys/allocator.h>
#include <task.h>
#include "filesystem.h"
#include "kfilesystem.h"

struct FileDescriptor *createFileDescriptor(
        s32 (*readFct)(void *, void *, u32),
        s32 (*writeFct)(void *, void *, u32),
        off_t (*seekFct)(void *, off_t, int),
        int (*closeFct)(void *),
        int (*statFct)(void *, struct stat *)
) {
    struct FileDescriptor *file = kmalloc(sizeof(struct FileDescriptor), 0, "filedescriptor");
    if (!file)
        return NULL;

    file->entryData = NULL;
    file->writeFct = writeFct;
    file->readFct = readFct;
    file->seekFct = seekFct;
    file->closeFct = closeFct;
    file->statFct = statFct;
    return file;
}

int open(const char *pathname, int flags) {
    if (!pathname)
        return -1;

    int id;
    for (id = 0; id < MAX_NB_FILE; id++)
        if (currentTask->lstFiles[id] == NULL)
            break;

    if (id >= MAX_NB_FILE)
        return -1;

    struct FileDescriptor *file = kfsOpen(pathname, flags);
    if (!file)
        return -1;

    currentTask->lstFiles[id] = file;
    return id;
}

int stat(const char *pathname, struct stat *data) {
    if (!pathname)
        return -1;
    return kfsStat(pathname, data);
}

static inline struct FileDescriptor *getFileDescriptorById(int fd) {
    if (fd >= MAX_NB_FILE)
        return NULL;
    return currentTask->lstFiles[fd];
}

s32 read(int fd, void *buf, u32 size) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->readFct)
        return -1;
    return file->readFct(file->entryData, buf, size);
}

s32 write(int fd, void *buf, u32 size) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->writeFct)
        return -1;
    return file->writeFct(file->entryData, buf, size);
}

off_t seek(int fd, off_t offset, int whence) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->seekFct)
        return -1;
    return file->seekFct(file->entryData, offset, whence);
}

int close(int fd) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->closeFct)
        return -1;

    file->closeFct(file->entryData);
    kfree(file);
    currentTask->lstFiles[fd] = NULL;
    return 0;
}

int fstat(int fd, struct stat *data) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->statFct)
        return -1;

    return file->statFct(file->entryData, data);
}

static inline struct FolderDescriptor *getFolderDescriptorById(int fd) {
    if (fd >= MAX_NB_FOLDER)
        return NULL;
    return currentTask->lstFolder[fd];
}

int opendir(const char *pathname) {
    if (!pathname)
        return -1;

    int id;
    for (id = 0; id < MAX_NB_FOLDER; id++)
        if (currentTask->lstFolder[id] == NULL)
            break;

    if (id >= MAX_NB_FOLDER)
        return -1;

    struct FolderDescriptor *folder = kfsOpendir(pathname);
    if (!folder)
        return -1;

    currentTask->lstFolder[id] = folder;
    return id;
}

struct dirent *readdir(int fd, struct dirent *data) {
    struct FolderDescriptor *folder = getFolderDescriptorById(fd);
    if (!folder || !folder->readFct)
        return NULL;

    return folder->readFct(folder->entryData, data);
}
int closedir(int fd) {
    struct FolderDescriptor *folder = getFolderDescriptorById(fd);
    if (!folder || !folder->closeFct)
        return -1;

    folder->closeFct(folder->entryData);
    kfree(folder);
    currentTask->lstFolder[fd] = NULL;
    return 0;
}