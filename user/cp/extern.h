//
// Created by rebut_p on 15/02/19.
//

#ifndef KERNEL_EXTERN_H
#define KERNEL_EXTERN_H

#include <fts.h>
#include <kstd.h>

typedef struct
{
    char *p_end;            /* pointer to NULL at end of path */
    char *target_end;        /* pointer to end of target base */
    char p_path[MAXPATHLEN];    /* pointer to the start of a path */
} PATH_T;

extern PATH_T to;
extern int fflag, iflag, lflag, nflag, pflag, sflag, vflag;

int copy_fifo(struct stat *, int);

int copy_file(const FTSENT *, int);

int copy_link(const FTSENT *, int);

int copy_special(struct stat *, int);

int setfile(struct stat *, int);

int preserve_dir_acls(struct stat *, char *, char *);

int preserve_fd_acls(int, int);

void usage(void);

#endif //KERNEL_EXTERN_H
