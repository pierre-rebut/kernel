/*
* Copyright (c) LSE
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY LSE AS IS AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL LSE BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <kstd.h>
#include <syscallw.h>

static inline u32 syscall0(int syscall_nb) {
    u32 res;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb));

    return res;
}

static inline u32 syscall1(int syscall_nb, u32 ebx) {
    u32 res;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb), "b"(ebx));

    return res;
}

static inline u32 syscall2(int syscall_nb, u32 ebx, u32 ecx) {
    u32 res;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb), "b"(ebx), "c"(ecx));

    return res;
}

static inline u32 syscall3(int syscall_nb, u32 ebx, u32 ecx, u32 edx) {
    u32 res;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb), "b"(ebx), "c"(ecx), "path"(edx));

    return res;
}

int exit(int value) {
    return ((int) syscall1(SYSCALL_EXIT, (u32) value));
}

void *sbrk(ssize_t increment) {
    return ((void *) syscall1(SYSCALL_SBRK, increment));
}

int getkey(void) {
    return ((int) syscall0(SYSCALL_GETKEY));
}

unsigned long gettick(void) {
    return ((unsigned long) syscall0(SYSCALL_GETTICK));
}

int open(const char *pathname, int flags) {
    return ((int) syscall2(SYSCALL_OPEN, (u32) pathname, flags));
}

ssize_t read(int fd, void *buf, size_t count) {
    return ((ssize_t) syscall3(SYSCALL_READ, fd, (u32) buf, count));
}

int write(int fd, const void *s, size_t length) {
    return ((int) syscall3(SYSCALL_WRITE, fd, (u32) s, length));
}

off_t seek(int filedes, off_t offset, int whence) {
    return ((off_t) syscall3(SYSCALL_SEEK, filedes, offset, whence));
}

int close(int fd) {
    return ((int) syscall1(SYSCALL_CLOSE, fd));
}

int playsound(struct melody *melody, int repeat) {
    return ((int) syscall2(SYSCALL_PLAYSOUND, (u32) melody, repeat));
}

int setvideo(int mode) {
    return ((int) syscall1(SYSCALL_SETVIDEO, mode));
}

void swap_frontbuffer(const void *buffer) {
    syscall1(SYSCALL_SWAP_FRONTBUFFER, (u32) buffer);
}

int getmouse(int *x, int *y, int *buttons) {
    return ((int) syscall3(SYSCALL_GETMOUSE, (u32) x, (u32) y, (u32) buttons));
}

int getkeymode(int mode) {
    return ((int) syscall1(SYSCALL_GETKEYMODE, mode));
}

int usleep(u32 duration) {
    return ((int) syscall1(SYSCALL_USLEEP, duration));
}

u32 kill(pid_t pid) {
    return syscall1(SYSCALL_KILL, (u32) pid);
}

int waitpid(pid_t pid) {
    return ((int) syscall1(SYSCALL_WAITPID, (u32) pid));
}

u32 getpid() {
    return syscall0(SYSCALL_GETPID);
}

int sleep(u32 duration) {
    return usleep(duration * 1000);
}

u32 execve(const struct ExceveInfo *info) {
    return syscall1(SYSCALL_EXECVE, (u32) info);
}

int stat(const char *pathname, struct stat *data) {
    return syscall2(SYSCALL_STAT, (u32) pathname, (u32) data);
}

int fstat(int fd, struct stat *data) {
    return syscall2(SYSCALL_FSTAT, (u32) fd, (u32) data);
}

int chdir(const char *path) {
    return syscall1(SYSCALL_CHDIR, (u32) path);
}

int opendir(const char *name) {
    return syscall1(SYSCALL_OPENDIR, (u32) name);
}

int closedir(int repertoire) {
    return syscall1(SYSCALL_CLOSEDIR, (u32) repertoire);
}

struct dirent *readdir(int repertoire) {
    static struct dirent data = {0};
    return (struct dirent *) syscall2(SYSCALL_READDIR, (u32) repertoire, (u32) &data);
}

int mount(char id, const char *fstype, u32 arg) {
    return syscall3(SYSCALL_MOUNT, (u32) id, (u32) fstype, arg);
}

int umount(char id) {
    return syscall1(SYSCALL_UMOUNT, (u32) id);
}

int pipe(int fd[2]) {
    return syscall1(SYSCALL_PIPE, (u32) fd);
}

int dup2(int o, int n) {
    return syscall2(SYSCALL_DUP2, (u32) o, (u32) n);
}

char *getcwd(char *buf, u32 size) {
    return (char *) syscall2(SYSCALL_GETCWD, (u32) buf, size);
}
