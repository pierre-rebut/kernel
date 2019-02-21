//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <types.h>
#include <errno.h>

int syscall0(int syscall_nb);

int syscall1(int syscall_nb, u32 ebx);

int syscall2(int syscall_nb, u32 ebx, u32 ecx);

int syscall3(int syscall_nb, u32 ebx, u32 ecx, u32 edx);

#define SYSCALL_EXIT            0
#define SYSCALL_BRK            1
#define SYSCALL_SBRK            2
#define SYSCALL_GETKEY            3
#define SYSCALL_GETTICK            4
#define SYSCALL_OPEN            5
#define SYSCALL_READ            6
#define SYSCALL_WRITE            7
#define SYSCALL_SEEK            8
#define SYSCALL_CLOSE            9
#define SYSCALL_SETVIDEO        10
#define SYSCALL_SWAP_FRONTBUFFER    11
#define SYSCALL_PLAYSOUND        12
#define SYSCALL_GETMOUSE        13 /* XXX: not implemented */
#define SYSCALL_USLEEP            14
#define SYSCALL_WAITPID            15
#define SYSCALL_KILL            16
#define SYSCALL_GETPID        17
#define SYSCALL_EXECVE        18
#define SYSCALL_STAT            19
#define SYSCALL_FSTAT            20
#define SYSCALL_CHDIR           21
#define SYSCALL_GETKEYMODE        22
#define SYSCALL_OPENDIR         23
#define SYSCALL_CLOSEDIR        24
#define SYSCALL_READDIR         25
#define SYSCALL_MOUNT           26
#define SYSCALL_UMOUNT          27
#define SYSCALL_PIPE            28
#define SYSCALL_DUP2            29
#define SYSCALL_GETCWD          30
#define SYSCALL_SYSCONF         31
#define SYSCALL_SYNC            32
#define SYSCALL_MKDIR           33
#define SYSCALL_MKFILE          34
#define SYSCALL_FCHDIR          35
#define SYSCALL_CHMOD           36
#define SYSCALL_FCHMOD          37
#define SYSCALL_LINK            38
#define SYSCALL_SYMLINK         39
#define SYSCALL_UNLINK          40
#define SYSCALL_ISATTY          41
#define SYSCALL_TIME            42
#define SYSCALL_FUTIMENS        43
#define SYSCALL_BEEP            44 // todo

#endif //KERNEL_SYSCALL_H
