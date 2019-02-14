//
// Created by rebut_p on 15/02/19.
//

#include "errno.h"

#define NB_ERRNO 29

static const char *errnoStr[] = {
        "Operation not permitted",
        "No such file or directory",
        "No such process",
        "Invalid system call number",
        "I/O error",
        "No such device or address",
        "Argument list too long",
        "Exec format error",
        "Bad file number",
        "No child processes",
        "Try again",
        "Out of memory",
        "Permission denied",
        "Bad address",
        "Block device required",
        "Device or resource busy",
        "File exists",
        "No such device",
        "Not a directory",
        "Is a directory",
        "Invalid argument",
        "Too many open files",
        "Illegal seek",
        "Read-only file system",
        "Broken pipe",
        "Resource deadlock would occur",
        "File name too long",
        "Directory not empty"
};

int errno = 0;

const char *strerror(u32 e) {
    e -= 1;

    if (e >= NB_ERRNO)
        return NULL;

    return errnoStr[e];
}