//
// Created by rebut_p on 10/02/19.
//

#ifndef KERNEL_SYSCALLW_H
#define KERNEL_SYSCALLW_H

#include <kstd.h>

/*
** syscalls
*/

#define SYSCALL_EXIT            0
#define SYSCALL_SBRK			1
#define SYSCALL_GETKEY			2
#define SYSCALL_GETTICK			3
#define SYSCALL_OPEN			4
#define SYSCALL_READ			5
#define SYSCALL_WRITE			6
#define SYSCALL_SEEK			7
#define SYSCALL_CLOSE			8
#define SYSCALL_SETVIDEO		9
#define SYSCALL_SWAP_FRONTBUFFER	10
#define SYSCALL_PLAYSOUND		11
#define SYSCALL_GETMOUSE		12 /* XXX: not implemented */
#define SYSCALL_USLEEP		    13
#define SYSCALL_WAITPID	    	14
#define SYSCALL_KILL		    15
#define SYSCALL_GETPID  		16
#define SYSCALL_EXECVE  		17
#define SYSCALL_STAT     		18
#define SYSCALL_FSTAT     		19
#define SYSCALL_CHDIR           20
#define SYSCALL_GETKEYMODE		21
#define SYSCALL_OPENDIR         22
#define SYSCALL_CLOSEDIR        23
#define SYSCALL_READDIR         24
#define SYSCALL_MOUNT           25
#define SYSCALL_UMOUNT          26
#define SYSCALL_FORK            27
#define SYSCALL_PIPE            28
#define SYSCALL_DUP2            29
#define SYSCALL_GETCWD          30

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
u32 kill(pid_t pid);
int waitpid(pid_t pid);
u32 getpid();
int sleep(u32 duration);
u32 execve(const struct ExceveInfo *info);
int stat(const char *pathname, struct stat *data);
int fstat(int fd, struct stat *data);
int chdir(const char *path);
int opendir(const char *name);
int closedir(int repertoire);
struct dirent* readdir(int repertoire);
int mount(char id, const char *fstype, u32 arg);
int umount(char id);
int pipe(int fd[2]);
int dup2(int o, int n);
char *getcwd(char *buf, u32 size);

#endif //KERNEL_SYSCALLW_H
