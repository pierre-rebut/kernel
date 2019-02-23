//
// Created by rebut_p on 16/02/19.
//

#include <unistd.h>

#include "syscalls.h"

uid_t getuid()
{
    return 0;
}

int isatty(int fd)
{
    return syscall1(SYSCALL_ISATTY, (u32) fd);
}

int exit(int value)
{
    return syscall1(SYSCALL_EXIT, (u32) value);
}

int brk(void *addr)
{
    return syscall1(SYSCALL_BRK, (u32) addr);
}

void *sbrk(ssize_t increment)
{
    return (void *) syscall1(SYSCALL_SBRK, (u32) increment);
}

int getkey(void)
{
    return syscall0(SYSCALL_GETKEY);
}

unsigned long gettick(void)
{
    return (unsigned long) syscall0(SYSCALL_GETTICK);
}

int open(const char *pathname, int flags, mode_t mode)
{
    return syscall3(SYSCALL_OPEN, (u32) pathname, (u32) flags, mode);
}

ssize_t read(int fd, void *buf, size_t count)
{
    return (ssize_t) syscall3(SYSCALL_READ, (u32) fd, (u32) buf, count);
}

int write(int fd, const void *s, size_t length)
{
    return syscall3(SYSCALL_WRITE, (u32) fd, (u32) s, length);
}

off_t seek(int filedes, off_t offset, int whence)
{
    return (off_t) syscall3(SYSCALL_SEEK, (u32) filedes, (u32) offset, (u32) whence);
}

int close(int fd)
{
    return syscall1(SYSCALL_CLOSE, (u32) fd);
}

int setvideo(int mode)
{
    return syscall1(SYSCALL_SETVIDEO, (u32) mode);
}

void swap_frontbuffer(const void *buffer)
{
    syscall1(SYSCALL_SWAP_FRONTBUFFER, (u32) buffer);
}

int getmouse(int *x, int *y, int *buttons)
{
    return syscall3(SYSCALL_GETMOUSE, (u32) x, (u32) y, (u32) buttons);
}

int getkeymode(int mode)
{
    return syscall1(SYSCALL_GETKEYMODE, (u32) mode);
}

int usleep(u32 duration)
{
    return syscall1(SYSCALL_USLEEP, duration);
}

int waitpid(pid_t pid)
{
    return syscall1(SYSCALL_WAITPID, (u32) pid);
}

pid_t getpid()
{
    return syscall0(SYSCALL_GETPID);
}

int sleep(u32 duration)
{
    return usleep(duration * 1000);
}

pid_t execve(const struct ExceveInfo *info)
{
    return syscall1(SYSCALL_EXECVE, (u32) info);
}

int chdir(const char *path)
{
    return syscall1(SYSCALL_CHDIR, (u32) path);
}

int pipe(int fd[2])
{
    return syscall1(SYSCALL_PIPE, (u32) fd);
}

int dup2(int o, int n)
{
    return syscall2(SYSCALL_DUP2, (u32) o, (u32) n);
}

char *getcwd(char *buf, u32 size)
{
    return (char *) syscall2(SYSCALL_GETCWD, (u32) buf, size);
}

long sysconf(int name)
{
    return (long) syscall1(SYSCALL_SYSCONF, (u32) name);
}

void sync()
{
    syscall0(SYSCALL_SYNC);
}

int fchdir(int fd)
{
    return syscall1(SYSCALL_FCHDIR, (u32) fd);
}

int link(const char *p1, const char *p2)
{
    return syscall2(SYSCALL_LINK, (u32) p1, (u32) p2);
}

int symlink(const char *p1, const char *p2)
{
    return syscall2(SYSCALL_SYMLINK, (u32) p1, (u32) p2);
}

int unlink(const char *p1)
{
    return syscall1(SYSCALL_UNLINK, (u32) p1);
}

int beep() {
    return syscall0(SYSCALL_BEEP);
}
