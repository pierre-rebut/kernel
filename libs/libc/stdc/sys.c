//
// Created by rebut_p on 22/02/19.
//

#include <sys/stat.h>

#include "syscalls.h"

/*
 * system/mount.h
 */

int mount(const char *fstype, const char *dev, const char *mnt)
{
    return syscall3(SYSCALL_MOUNT, (u32) fstype, (u32) dev, (u32) mnt);
}

int umount(const char *mnt)
{
    return syscall1(SYSCALL_UMOUNT, (u32) mnt);
}

/*
 * system/stat.h
 */

int stat(const char *pathname, struct stat *data)
{
    return syscall2(SYSCALL_STAT, (u32) pathname, (u32) data);
}

int fstat(int fd, struct stat *data)
{
    return syscall2(SYSCALL_FSTAT, (u32) fd, (u32) data);
}

int mkdir(const char *file, mode_t mode)
{
    return syscall2(SYSCALL_MKDIR, (u32) file, mode);
}

int mkfile(const char *file, mode_t mode)
{
    return syscall2(SYSCALL_MKFILE, (u32) file, (u32) mode);
}

int futimens(int fd, time_t time[2]) {
    return syscall2(SYSCALL_FUTIMENS, (u32) fd, (u32) time);
}

int chmod(const char *path, mode_t mode)
{
    return syscall2(SYSCALL_CHMOD, (u32) path, mode);
}

int fchmod(int fd, mode_t mode)
{
    return syscall2(SYSCALL_FCHMOD, (u32) fd, mode);
}