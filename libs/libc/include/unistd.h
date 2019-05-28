//
// Created by rebut_p on 10/02/19.
//

#ifndef _UNISTD_H
#define _UNISTD_H

#include "ctype.h"

char *getenv(const char *name);

mode_t umask(mode_t mode);

int access(const char *name, int amode);

int mkstemp(char *template);

int getpagesize();

/*
** syscalls
*/

int exit(int value);

int brk(void *addr);

void *sbrk(ssize_t increment);

int getkey(void);

unsigned long gettick(void);

int open(const char *pathname, int flags, mode_t mode);

ssize_t read(int fd, void *buf, size_t count);

int write(int fd, const void *s, size_t length);

off_t seek(int filedes, off_t offset, int whence);

int close(int fd);

int setvideo(int mode);

void swap_frontbuffer(const void *buffer);


int getmouse(int *x, int *y, int *buttons);

int getkeymode(int mode);

int uslepp(u32 duration);

int waitpid(pid_t pid);

pid_t getpid();

int sleep(u32 duration);

pid_t execve(const struct ExceveInfo *info);

int chdir(const char *path);

int pipe(int fd[2]);

int dup2(int o, int n);

char *getcwd(char *buf, u32 size);

long sysconf(int name);

void sync();

int fchdir(int fd);

int link(const char *, const char *);

int symlink(const char *, const char *);

int unlink(const char *name);

uid_t getuid();

#define geteuid getuid

int isatty(int fd);

#define fchown(fd, owner, grp) 0 // todo

#define chown(path, owner, grp) 0

int beep();

pid_t fork();

#endif //_UNISTD_H
