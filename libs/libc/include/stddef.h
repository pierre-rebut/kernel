//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_STDDEF_H
#define KERNEL_STDDEF_H

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;

typedef u32 mode_t;
typedef u32 size_t;
typedef s32 ssize_t;
typedef s32 off_t;
typedef s32 pid_t;
typedef u32 uid_t;
typedef u32 gid_t;
typedef s32 time_t;

typedef unsigned short int u_short;
typedef unsigned long int u_long;
typedef unsigned int u_int;

typedef unsigned char bool;

#define false 0
#define true 1

#define NULL ((void*)0)

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define CVTBUFSIZE        (309 + 43)
#define ASCBUFSIZE        (26 + 2)

#endif //KERNEL_STDDEF_H
