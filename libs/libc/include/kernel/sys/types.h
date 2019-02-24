//
// Created by rebut_p on 23/02/19.
//

#ifndef _KERNEL_TYPES_H
#define _KERNEL_TYPES_H

enum FileType
{
    FT_DIRECTORY,
    FT_FILE
};

struct stat
{
    u32 st_ino;         /* Inode number */
    u16 st_mode;        /* File type and mode */
    u16 st_nlink;       /* Number of hard links */
    u16 st_uid;         /* User ID of owner */
    u16 st_gid;         /* Group ID of owner */
    u32 st_size;        /* Total size, in bytes */
    u32 st_blocks;
    u32 st_blksize;     /* Block size for filesystem I/O */

    time_t st_atim; // time last access
    time_t st_mtim; // time last modified
    time_t st_ctim; // time last status changed
};

struct dirent
{
    u32 d_ino;
    u32 d_off;       // décalage jusqu'à la dirent suivante
    unsigned short d_reclen;    // longueur de cet enregistrement
    enum FileType d_type;      // type du fichier
    u32 d_namlen;
    char d_name[256]; // nom du fichier
} __attribute__((packed));

#define DIRENT_BUFFER_NB 15
#define DIRENT_BUFFER_SIZE (DIRENT_BUFFER_NB * sizeof(struct dirent))

/* misc */
#define O_WRONLY    0x1
#define O_RDONLY    0x2
#define O_RDWR      O_RDONLY | O_WRONLY
#define O_CREAT     0x4
#define O_APPEND    0x8
#define O_TRUNC     0x10
#define O_EXCL      0x20

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif //_KERNEL_TYPES_H
