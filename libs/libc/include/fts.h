//
// Created by rebut_p on 15/02/19.
//

#ifndef _FTS_H
#define _FTS_H

#include <sys/stat.h>

typedef struct
{
    struct _ftsent *fts_cur;    /* current node */
    struct _ftsent *fts_child;    /* linked list of children */
    struct _ftsent **fts_array;    /* sort array */
    unsigned long int fts_dev;            /* starting device # */
    char *fts_path;            /* path for this descent */
    int fts_rfd;            /* fd for root */
    int fts_pathlen;        /* sizeof(path) */
    int fts_nitems;            /* elements in the sort array */
    int (*fts_compar)(const void *, const void *); /* compare fn */

#define    FTS_COMFOLLOW    0x0001        /* follow command line symlinks */
#define    FTS_LOGICAL    0x0002        /* logical walk */
#define    FTS_NOCHDIR    0x0004        /* don't change directories */
#define    FTS_NOSTAT    0x0008        /* don't get stat info */
#define    FTS_PHYSICAL    0x0010        /* physical walk */
#define    FTS_SEEDOT    0x0020        /* return dot and dot-dot */
#define    FTS_XDEV    0x0040        /* don't cross devices */
#define FTS_WHITEOUT    0x0080        /* return whiteout information */
#define    FTS_COMFOLLOWDIR 0x400
#define    FTS_OPTIONMASK    0x00ff        /* valid user option mask */

#define    FTS_NAMEONLY    0x0100        /* (private) child names only */
#define    FTS_STOP    0x0200        /* (private) unrecoverable error */
    int fts_options;        /* fts_open options, global flags */
} FTS;

typedef struct _ftsent
{
    struct _ftsent *fts_cycle;    /* cycle node */
    struct _ftsent *fts_parent;    /* parent directory */
    struct _ftsent *fts_link;    /* next file in directory */
    long fts_number;            /* local numeric value */
    void *fts_pointer;            /* local address value */
    char *fts_accpath;        /* access path */
    char *fts_path;            /* root path */
    int fts_errno;            /* errno for this node */
    int fts_symfd;            /* fd for symlink */
    unsigned short fts_pathlen;    /* strlen(fts_path) */
    unsigned short fts_namelen;    /* strlen(fts_name) */

    unsigned long int fts_ino;            /* inode */
    unsigned long int fts_dev;            /* device */
    unsigned long int fts_nlink;        /* link count */

#define    FTS_ROOTPARENTLEVEL    -1
#define    FTS_ROOTLEVEL         0
    short fts_level;        /* depth (-1 to N) */

#define    FTS_D         1        /* preorder directory */
#define    FTS_DC         2        /* directory that causes cycles */
#define    FTS_DEFAULT     3        /* none of the above */
#define    FTS_DNR         4        /* unreadable directory */
#define    FTS_DOT         5        /* dot or dot-dot */
#define    FTS_DP         6        /* postorder directory */
#define    FTS_ERR         7        /* error; errno is set */
#define    FTS_F         8        /* regular file */
#define    FTS_INIT     9        /* initialized only */
#define    FTS_NS        10        /* stat(2) failed */
#define    FTS_NSOK    11        /* no stat(2) requested */
#define    FTS_SL        12        /* symbolic link */
#define    FTS_SLNONE    13        /* symbolic link without target */
#define FTS_W        14        /* whiteout object */
    unsigned short fts_info;    /* user flags for FTSENT structure */

#define    FTS_DONTCHDIR     0x01        /* don't chdir .. to the parent */
#define    FTS_SYMFOLLOW     0x02        /* followed a symlink to get here */
    unsigned short fts_flags;    /* private flags for FTSENT structure */

#define    FTS_AGAIN     1        /* read node again */
#define    FTS_FOLLOW     2        /* follow symbolic link */
#define    FTS_NOINSTR     3        /* no instructions */
#define    FTS_SKIP     4        /* discard node */
    unsigned short fts_instr;    /* fts_set() instructions */

    struct stat *fts_statp;        /* stat(2) information */
    char fts_name[1];        /* file name */
} FTSENT;


FTSENT *fts_children(FTS *, int);

int fts_close(FTS *);

FTS *fts_open(char *const *, int, int (*)(const FTSENT **, const FTSENT **));

FTSENT *fts_read(FTS *);

int fts_set(FTS *, FTSENT *, int);

#endif //_FTS_H
