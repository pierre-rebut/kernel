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

struct stat {
    u32 inumber;
    u32 file_sz;
    u32 idx;
    u32 blk_cnt;
    u32 next_inode;
    u32 d_blk_cnt;
    u32 i_blk_cnt;
    u32 cksum;
};

struct dirent {
    u32          d_ino;
    u32          d_off;       /* décalage jusqu'à la dirent suivante */
    unsigned short d_reclen;    /* longueur de cet enregistrement */
    unsigned char  d_type;      /* type du fichier */
    char           d_name[256]; /* nom du fichier */
};

#endif
