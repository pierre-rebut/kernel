//
// Created by rebut_p on 16/02/19.
//

#ifndef _DIRENT_H
#define _DIRENT_H

#include "ctype.h"
#include "sys/stat.h"

typedef struct
{
    int fd;

    u32 size;
    u32 nblock;
    u32 offset;

    struct dirent data;
    u8 block[DIRENT_BUFFER_SIZE];
} DIR;

DIR *opendir(const char *name);

int closedir(DIR *dir);

struct dirent *readdir(DIR *dir);

#endif //_DIRENT_H
