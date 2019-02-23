//
// Created by rebut_p on 23/02/19.
//

#ifndef _KERNEL_CTYPE_H
#define _KERNEL_CTYPE_H

#include "stddef.h"

#define isspace(c) ((c) == '\t' || (c) == '\n' || (c) == '\v' || (c) == '\f' || (c) == '\r' || (c) == ' ')
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define isascii(c) ((c) >= 0 && (c) < 128)
#define isprint(c) ((c) >= 20 && (c) <= 126)
#define isalpha(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' || (c) <= 'z'))
#define isxdigit(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isblank(c) ((c) == '\t' || (c) == ' ')
#define ispunct(c) ((c) == '.' || (c) == '!' || (c) == '?' || (c) == ',' || (c) == ';' || (c) == ':')
#define iscntrl(c) 1 // todo

#define tolower(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 32 : (c))
#define toupper(c) ((c) >= 'a' && (c) <= 'z' ? (c) - 32 : (c))

#define howmany(x, y)  (((x) + ((y) - 1)) / (y))

#define    __S_IFMT    0170000
#define    __S_ISTYPE(mode, mask)    (((mode) & __S_IFMT) == (mask))

#define    __S_IFDIR    0040000    /* Directory.  */
#define    __S_IFCHR    0020000    /* Character device.  */
#define    __S_IFBLK    0060000    /* Block device.  */
#define    __S_IFREG    0100000    /* Regular file.  */
#define    __S_IFIFO    0010000    /* FIFO.  */
#define    __S_IFLNK    0120000    /* Symbolic link.  */
#define    __S_IFSOCK    0140000    /* Socket.  */

#define S_ISDIR(mode)     __S_ISTYPE((mode), __S_IFDIR)
#define S_ISCHR(mode)     __S_ISTYPE((mode), __S_IFCHR)
#define S_ISBLK(mode)     __S_ISTYPE((mode), __S_IFBLK)
#define S_ISREG(mode)     __S_ISTYPE((mode), __S_IFREG)
#define S_ISFIFO(mode)    __S_ISTYPE((mode), __S_IFIFO)
#define S_ISLNK(mode)     __S_ISTYPE((mode), __S_IFLNK)

#define S_IFREG __S_IFREG
#define S_IFIFO __S_IFIFO
#define S_IFSOCK __S_IFSOCK
#define S_IFCHR __S_IFCHR
#define S_IFBLK __S_IFBLK
#define S_IFDIR __S_IFDIR
#define S_IFLNK __S_IFLNK
#define S_IFMT __S_IFMT

#define    S_ISUID    0x4000    /* Set user ID on execution.  */
#define    S_ISGID    0x2000    /* Set group ID on execution.  */
#define    S_ISVTX    0x1000    /* Save swapped text after use (sticky).  */

#define    S_IRUSR    0x100    /* Read by owner.  */
#define    S_IWUSR    0x80    /* Write by owner.  */
#define    S_IXUSR    0x40    /* Execute by owner.  */
#define    S_IRGRP    0x20    /* Read by groupe.  */
#define    S_IWGRP    0x10    /* Write by groupe.  */
#define    S_IXGRP    0x8    /* Execute by groupe.  */
#define    S_IROTH    0x4    /* Read by others.  */
#define    S_IWOTH    0x2    /* Write by others.  */
#define    S_IXOTH    0x1    /* Execute by others.  */

#define R_OK S_IXUSR
#define W_OK S_IWUSR
#define X_OK S_IXUSR
#define F_OK 0

#define    S_IRWXU    (S_IRUSR|S_IWUSR|S_IXUSR) /* Read, write, and execute by owner.  */
#define    S_IRWXG    (S_IRWXU >> 3) /* Read, write, and execute by group.  */
#define    S_IRWXO    (S_IRWXG >> 3) /* Read, write, and execute by others.  */

struct ExceveInfo
{
    const char *cmdline;
    const char **av;
    const char **env;

    int fd_in;
    int fd_out;
};

#define INIT_EXECINFO() {0, 0, 0, -1, -1}

enum SysConfigInfo
{
    _SC_PHYS_PAGES,
    _SC_PAGESIZE
};

enum e_cons_codes
{
    CONS_ESCAPE = 255,
    CONS_CLEAR = 1,
    CONS_COLOR = 2,
    CONS_SETX = 3,
    CONS_SETY = 4,
    CONS_BLACK = 0,
    CONS_BLUE = 1,
    CONS_GREEN = 2,
    CONS_CYAN = 3,
    CONS_RED = 4,
    CONS_MAGENTA = 5,
    CONS_YELLOW = 6,
    CONS_WHITE = 7,
    CONS_BLINK = (1 << 7),
    CONS_LIGHT = (1 << 3)
};

#define MAXPATHLEN 255
#define VIDEO_GRAPHIC    0
#define VIDEO_TEXT    1

#endif //_KERNEL_CTYPE_H
