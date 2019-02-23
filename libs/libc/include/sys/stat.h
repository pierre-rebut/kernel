//
// Created by rebut_p on 22/02/19.
//

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <stddef.h>
#include "types.h"

int futimens(int fd, time_t time[2]);

int stat(const char *pathname, struct stat *data);

int fstat(int fd, struct stat *data);

int mkfile(const char *, mode_t mode);

int mkdir(const char *, mode_t mode);

int chmod(const char *path, mode_t mode);

int fchmod(int, mode_t mode);

#endif //_STAT_H
