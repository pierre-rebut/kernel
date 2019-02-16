//
// Created by rebut_p on 17/02/19.
//

#include "syscalls.h"

int syscall0(int syscall_nb) {
    int res;
    errno = 0;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb));

    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}

int syscall1(int syscall_nb, u32 ebx) {
    int res;
    errno = 0;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb), "b"(ebx));

    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}

int syscall2(int syscall_nb, u32 ebx, u32 ecx) {
    int res;
    errno = 0;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb), "b"(ebx), "c"(ecx));

    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}

int syscall3(int syscall_nb, u32 ebx, u32 ecx, u32 edx) {
    int res;
    errno = 0;

    asm volatile ("int $0x80" : "=a"(res) : "a"(syscall_nb), "b"(ebx), "c"(ecx), "d"(edx));

    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}