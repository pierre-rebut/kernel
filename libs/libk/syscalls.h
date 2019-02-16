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
#define SYSCALL_PIPE            27
#define SYSCALL_DUP2            28
#define SYSCALL_GETCWD          29
#define SYSCALL_SYSCONF         30
#define SYSCALL_SYNC            31
#define SYSCALL_MKDIR           32
#define SYSCALL_MKFILE          33
#define SYSCALL_FCHDIR          34
#define SYSCALL_CHMOD           35
#define SYSCALL_FCHMOD          36
#define SYSCALL_LINK            37
#define SYSCALL_SYMLINK         38
#define SYSCALL_UNLINK          39

#endif //KERNEL_SYSCALL_H
