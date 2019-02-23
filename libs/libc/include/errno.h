//
// Created by rebut_p on 15/02/19.
//

#ifndef _ERRNO_H
#define _ERRNO_H

#include <kernel/errno-base.h>

#include "ctype.h"

extern int errno;

const char *strerror(int e);

#endif //_ERRNO_H
