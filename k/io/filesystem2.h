//
// Created by rebut_p on 15/12/18.
//

#ifndef KERNEL_FILESYSTEM_H
#define KERNEL_FILESYSTEM_H

#include <k/types.h>
typedef s32 off_t;

#define MAX_NB_FILE 255
#define MAX_NB_FOLDER 50
/*
struct stat {
    u32 inumber;
    u32 file_sz;
    u32 idx;
    u32 blk_cnt;
    u32 d_blk_cnt;
    u32 i_blk_cnt;
    u32 cksum;
};

struct dirent {
    u32          d_ino;
    u32          d_off;       // décalage jusqu'à la dirent suivante
    unsigned short d_reclen;    // longueur de cet enregistrement
    unsigned char  d_type;      // type du fichier
    char           d_name[256]; // nom du fichier
};

struct Fs {
    char *name;

    void *(*open)(const char *pathname, int flags);
    s32 (*read)(void *, void *buf, u32 size);
    s32 (*write)(void *, void *buf, u32 size);
    off_t (*seek)(void *, off_t offset, int whence);
    int (*close)(void *);
    int (*stat)(void *, struct stat *data);
    struct dirent *(*readdir)(void *, struct dirent *);

    struct Fs *next;
};

struct FileDescriptor {
    void *entryData;
    struct Fs *Fs;
};

void fsRegister(struct Fs *Fs);

int open(const char *pathname, int flags);
s32 read(int fd, void *buf, u32 size);
s32 write(int fd, void *buf, u32 size);
off_t seek(int fd, off_t offset, int whence);
int close(int fd);

int stat(const char *pathname, struct stat *data);
int fstat(int fd, struct stat *data);

int opendir(const char *pathname);
struct dirent *readdir(int fd, struct dirent *data);
int closedir(int fd);

struct FileDescriptor *createFileDescriptor(void *entryData, struct Fs *Fs); */

struct Fs {
    char *name;

    struct fs_dirent *(*root) (struct fs_volume * d);
    struct fs_volume *(*mount) (int unit);
    int (*umount) (struct fs_volume * d);
    int (*mkfs) (int unit);
    int (*close) (struct fs_dirent * d);
    int (*mkdir) (struct fs_dirent * d, const char *name);
    int (*mkfile) (struct fs_dirent * d, const char *name);

    struct fs_dirent *(*lookup) (struct fs_dirent * d, const char *name);
    int (*readdir) (struct fs_dirent * d, char *buffer, int buffer_length);
    int (*rmdir) (struct fs_dirent * d, const char *name);
    int (*link) (struct fs_dirent * d, const char *oldpath, const char *newpath);
    int (*unlink) (struct fs_dirent * d, const char *name);
    int (*read_block) (struct fs_dirent * d, char *buffer, u32 blocknum);
    int (*write_block) (struct fs_dirent * d, const char *buffer, u32 blocknum);
    int (*resize) (struct fs_dirent * d, u32 blocks);
    int (*compare) (struct fs_dirent * d1, struct fs_dirent * d2, int *result);

    struct Fs *next;
};

struct fs_volume {
    struct Fs *fs;
    u32 block_size;
    int refcount;
    void *private_data;
};

struct fs_dirent {
    struct fs_volume *v;
    u32 size;
    int refcount;
    void *private_data;
};

struct fs_file {
    struct fs_dirent *d;
    u32 size;
    int mode;
    int refcount;
    void *private_data;
};

struct fs_dirent *fs_resolve(const char *path);

void fs_register(struct Fs *f);
struct Fs *fs_lookup(const char *name);
int fs_mkfs(struct Fs *f, u32 device_no);

struct fs_volume *fs_volume_open(struct Fs *f, u32 device_no);
struct fs_volume *fs_volume_addref(struct fs_volume *v);
struct fs_dirent *fs_volume_root(struct fs_volume *);
int fs_volume_close(struct fs_volume *v);

struct fs_file *fs_file_open(struct fs_dirent *d, int mode);
struct fs_file *fs_file_addref(struct fs_file *f);
int fs_file_read(struct fs_file *f, char *buffer, u32 length, u32 offset);
int fs_file_write(struct fs_file *f, const char *buffer, u32 length, u32 offset);
int fs_file_close(struct fs_file *f);

// Sets dimensions[0] to the file size
int fs_file_get_dimensions(struct fs_file *f, int *dims, int n);

struct fs_dirent *fs_dirent_namei(struct fs_dirent *d, const char *path);
struct fs_dirent *fs_dirent_addref(struct fs_dirent *d);
int fs_dirent_readdir(struct fs_dirent *d, char *buffer, int buffer_length);
int fs_dirent_rmdir(struct fs_dirent *d, const char *name);
int fs_dirent_link(struct fs_dirent *d, const char *oldpath, const char *newpath);
int fs_dirent_unlink(struct fs_dirent *d, const char *name);
int fs_dirent_mkdir(struct fs_dirent *d, const char *name);
int fs_dirent_mkfile(struct fs_dirent *d, const char *name);
int fs_dirent_compare(struct fs_dirent *d1, struct fs_dirent *d2, int *result);
int fs_dirent_close(struct fs_dirent *d);

#endif //KERNEL_FILESYSTEM_H
