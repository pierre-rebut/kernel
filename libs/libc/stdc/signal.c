//
// Created by rebut_p on 22/02/19.
//

#include <signal.h>
#include "syscalls.h"

int kill(pid_t pid, int signal)
{
    return syscall2(SYSCALL_KILL, (u32) pid, (u32) signal);
}