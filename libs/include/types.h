#ifndef K_TYPES_H
#define K_TYPES_H

#include <stddef.h>

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
typedef u32 time_t;

#define isascii(c) ((c) >= 0 && (c) < 128)
#define isprint(c) ((c) >= 20 && (c) <= 126)

#define	__S_IFMT	0170000
#define	__S_ISTYPE(mode, mask)	(((mode) & __S_IFMT) == (mask))

#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */
#define	__S_IFLNK	0120000	/* Symbolic link.  */
#define	__S_IFSOCK	0140000	/* Socket.  */

#define	S_ISDIR(mode)	 __S_ISTYPE((mode), __S_IFDIR)
#define	S_ISCHR(mode)	 __S_ISTYPE((mode), __S_IFCHR)
#define	S_ISBLK(mode)	 __S_ISTYPE((mode), __S_IFBLK)
#define	S_ISREG(mode)	 __S_ISTYPE((mode), __S_IFREG)
# define S_ISFIFO(mode)	 __S_ISTYPE((mode), __S_IFIFO)
# define S_ISLNK(mode)	 __S_ISTYPE((mode), __S_IFLNK)

enum FileType {
    FT_DIRECTORY,
    FT_FILE
};

struct stat {
    u32 st_ino;         /* Inode number */
    u16 st_mode;        /* File type and mode */
    u16 st_nlink;       /* Number of hard links */
    u16 st_uid;         /* User ID of owner */
    u16 st_gid;         /* Group ID of owner */
    u32 st_size;        /* Total size, in bytes */
    u32 st_blksize;     /* Block size for filesystem I/O */

    time_t st_atim; // time last access
    time_t st_mtim; // time last modified
    time_t st_ctim; // time last status changed
};

struct dirent {
    u32 d_ino;
    u32 d_off;       // décalage jusqu'à la dirent suivante
    unsigned short d_reclen;    // longueur de cet enregistrement
    enum FileType d_type;      // type du fichier
    char d_name[256]; // nom du fichier
};

#endif
