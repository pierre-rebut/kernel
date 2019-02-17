//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_LS_H
#define KERNEL_LS_H

#include <stddef.h>
#include <fts.h>

#define NO_PRINT	1

#define HUMANVALSTR_LEN	5

extern long blocksize;		/* block size units */

extern int f_accesstime;	/* use time of last access */
extern int f_birthtime;	/* use time of file creation */
extern int f_flags;		/* show flags associated with a file */
extern int f_humanval;		/* show human-readable file sizes */
extern int f_label;		/* show MAC label */
extern int f_inode;		/* print inode */
extern int f_longform;		/* long listing format */
extern int f_octal;		/* print unprintables in octal */
extern int f_octal_escape;	/* like f_octal but use C escapes if possible */
extern int f_nonprint;		/* show unprintables as ? */
extern int f_samesort;		/* sort time and name in same direction */
extern int f_sectime;		/* print the real time for all files */
extern int f_size;		/* list size in short listing */
extern int f_slash;		/* append a '/' if the file is a directory */
extern int f_sortacross;	/* sort across rows, not down columns */
extern int f_statustime;	/* use time of last mode change */
extern int f_thousands;		/* show file sizes with thousands separators */
extern char *f_timeformat;	/* user-specified time format */
extern int f_notabs;		/* don't use tab-separated multi-col output */
extern int f_type;		/* add type character for non-regular files */
#ifdef COLORLS
extern int f_color;		/* add type in color for non-regular files */
#endif

typedef struct {
    FTSENT *list;
    u_long btotal;
    int entries;
    int maxlen;
    u32 s_block;
    u32 s_flags;
    u32 s_label;
    u32 s_group;
    u32 s_inode;
    u32 s_nlink;
    u32 s_size;
    u32 s_user;
} DISPLAY;

typedef struct {
    char *user;
    char *group;
    char *flags;
    char *label;
    char data[1];
} NAMES;


#endif //KERNEL_LS_H
