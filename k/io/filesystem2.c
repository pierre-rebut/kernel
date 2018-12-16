//
// Created by rebut_p on 15/12/18.
//

#include <sys/allocator.h>
#include <task.h>
#include <string.h>
#include "filesystem2.h"

static struct Fs *filesystemLists = NULL;

struct FileDescriptor *createFileDescriptor(void *entryData, struct Fs *fs) {
    struct FileDescriptor *file = kmalloc(sizeof(struct FileDescriptor), 0, "filedescriptor");
    if (!file)
        return NULL;

    file->entryData = entryData;
    file->fs = fs;
    return file;
}

static struct Fs *getFileSystemByName(const char *name) {
    struct Fs *tmp = filesystemLists;
    while (tmp != NULL) {
        if (strcmp(tmp->name, name) == 0)
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
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

    struct Fs *fs = getFileSystemByName("kfs");
    if (fs == NULL)
        return -1;

    void *data = fs->open(pathname, flags);

    struct FileDescriptor *file = createFileDescriptor(data, fs);
    if (!file)
        return -1;

    currentTask->lstFiles[id] = file;
    return id;
}

int stat(const char *pathname, struct stat *data) {
    if (!pathname)
        return -1;

    struct Fs *fs = getFileSystemByName("kfs");
    if (fs == NULL || fs->stat == NULL)
        return -1;
    return fs->stat(pathname, data);
}

static inline struct FileDescriptor *getFileDescriptorById(int fd) {
    if (fd >= MAX_NB_FILE)
        return NULL;
    return currentTask->lstFiles[fd];
}

s32 read(int fd, void *buf, u32 size) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->fs->read)
        return -1;
    return file->fs->read(file->entryData, buf, size);
}

s32 write(int fd, void *buf, u32 size) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->fs->write)
        return -1;
    return file->fs->write(file->entryData, buf, size);
}

off_t seek(int fd, off_t offset, int whence) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->fs->seek)
        return -1;
    return file->fs->seek(file->entryData, offset, whence);
}

int close(int fd) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->fs->close)
        return -1;

    file->fs->close(file->entryData);
    kfree(file);
    currentTask->lstFiles[fd] = NULL;
    return 0;
}

int fstat(int fd, struct stat *data) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->fs->stat)
        return -1;

    return file->fs->stat(file->entryData, data);
}

int opendir(const char *pathname) {
    return open(pathname, O_RDONLY);
}

struct dirent *readdir(int fd, struct dirent *data) {
    struct FileDescriptor *file = getFileDescriptorById(fd);
    if (!file || !file->fs->readdir)
        return NULL;

    return file->fs->readdir(file->entryData, data);
}

int closedir(int fd) {
    return close(fd);
}