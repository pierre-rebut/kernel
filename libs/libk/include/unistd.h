//
// Created by rebut_p on 10/02/19.
//

#ifndef KERNEL_SYSCALLW_H
#define KERNEL_SYSCALLW_H

#include <kstd.h>

/*
** syscalls
*/

int exit(int value);
void *sbrk(ssize_t increment);
int getkey(void);
unsigned long gettick(void);
int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t count);
int write(int fd, const void *s, size_t length);
off_t seek(int filedes, off_t offset, int whence);
int close(int fd);
int setvideo(int mode);
void swap_frontbuffer(const void *buffer);
int playsound(struct melody *melody, int repeat);
int getmouse(int *x, int *y, int *buttons);
int getkeymode(int mode);
int uslepp(u32 duration);
int kill(pid_t pid);
int waitpid(pid_t pid);
pid_t getpid();
int sleep(u32 duration);
pid_t execve(const struct ExceveInfo *info);
int stat(const char *pathname, struct stat *data);
int fstat(int fd, struct stat *data);
int chdir(const char *path);

int mount(const char *fstype, const char *dev, const char *mnt);
int umount(const char *mnt);
int pipe(int fd[2]);
int dup2(int o, int n);
char *getcwd(char *buf, u32 size);
long sysconf(int name);
void sync();
int mkfile(const char *);
int mkdir(const char *, mode_t mode);
int fchdir(int fd);
int chmod(const char *path, mode_t mode);
int fchmod(int, mode_t mode);
int link(const char *, const char *);
int symlink(const char *, const char *);
int unlink(const char *name);

#endif //KERNEL_SYSCALLW_H
