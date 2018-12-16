//
// Created by rebut_p on 17/12/18.
//

#include <string.h>
#include <sys/allocator.h>
#include <sys/physical-memory.h>
#include <task.h>
#include "filesystem2.h"

static struct Fs *fs_list = 0;

struct fs_dirent *fs_resolve(const char *path) {
    if (path[0] == '/') {
        return fs_dirent_namei(freeTimeTask->currentDir, &path[1]);
    } else {
        return fs_dirent_namei(currentTask->currentDir, path);
    }
}

void fs_register(struct Fs *f) {
    f->next = fs_list;
    fs_list = f;
}

struct Fs *fs_lookup(const char *name) {
    struct Fs *f;

    for (f = fs_list; f; f = f->next) {
        if (!strcmp(name, f->name)) {
            return f;
        }
    }
    return 0;
}

int fs_mkfs(struct Fs *f, u32 device_no) {
    if (!f->mkfs)
        return -1;
    return f->mkfs(device_no);
}

struct fs_volume *fs_volume_open(struct Fs *f, u32 device_no) {
    if (!f->mount)
        return 0;
    struct fs_volume *v = f->mount(device_no);
    if (v)
        v->fs = f;
    return v;
}

struct fs_volume *fs_volume_addref(struct fs_volume *v) {
    v->refcount++;
    return v;
}

int fs_volume_close(struct fs_volume *v) {
    if (!v->fs->umount)
        return -1;

    v->refcount--;
    if (v->refcount <= 0)
        return v->fs->umount(v);
    return -1;
}

struct fs_dirent *fs_volume_root(struct fs_volume *v) {
    if (!v->fs->root)
        return 0;

    struct fs_dirent *d = v->fs->root(v);
    d->v = fs_volume_addref(v);
    return d;
}

int fs_dirent_readdir(struct fs_dirent *d, char *buffer, int buffer_length) {
    if (!d->v->fs->readdir)
        return -1;
    return d->v->fs->readdir(d, buffer, buffer_length);
}

static struct fs_dirent *fs_dirent_lookup(struct fs_dirent *d, const char *name) {
    if (!d->v->fs->lookup)
        return 0;

    struct fs_dirent *r = d->v->fs->lookup(d, name);
    r->v = fs_volume_addref(d->v);
    return r;
}

int fs_dirent_compare(struct fs_dirent *d1, struct fs_dirent *d2, int *result) {
    if (!d1->v->fs->compare)
        return -1;

    return d1->v->fs->compare(d1, d2, result);
}

struct fs_dirent *fs_dirent_namei(struct fs_dirent *d, const char *path) {
    if (!d || !path)
        return 0;

    char *lpath = kmalloc(strlen(path) + 1, 0, "tmpPath");
    strcpy(lpath, path);

    char *part = strtok(lpath, "/");
    while (part) {
        d = fs_dirent_lookup(d, part);
        if (!d)
            break;

        part = strtok(0, "/");
    }
    kfree(lpath);
    return d;
}

struct fs_dirent *fs_dirent_addref(struct fs_dirent *d) {
    d->refcount++;
    return d;
}

int fs_dirent_close(struct fs_dirent *d) {
    if (!d->v->fs->close)
        return -1;

    d->refcount--;
    if (d->refcount <= 0) {
        struct fs_volume *v = d->v;
        d->v->fs->close(d);
        fs_volume_close(v);
    }

    return 0;
}

struct fs_file *fs_file_open(struct fs_dirent *d, int mode) {
    struct fs_file *f = kmalloc(sizeof(*f), 0, "fsFile");
    f->size = d->size;
    f->d = fs_dirent_addref(d);
    f->private_data = 0;
    f->mode = mode;
    f->refcount = 1;
    return f;
}

struct fs_file *fs_file_addref(struct fs_file *f) {
    f->refcount++;
    return f;
}

int fs_file_close(struct fs_file *f) {
    if (!f)
        return 0;
    f->refcount--;
    if (f->refcount <= 0) {
        fs_dirent_close(f->d);
        kfree(f);
    }
    return 0;
}

int fs_file_read(struct fs_file *file, char *buffer, u32 length, u32 offset) {
    int total = 0;
    int bs = file->d->v->block_size;

    if (!file->d->v->fs->read_block)
        return -1;

    if (offset > file->size) {
        return 0;
    }

    if (offset + length > file->size) {
        length = file->size - offset;
    }

    char *temp = kmalloc(4096, PAGESIZE, "fsFileRead");
    if (!temp)
        return -1;

    while (length > 0) {

        int blocknum = offset / bs;
        int actual = 0;

        if (offset % bs) {
            actual = file->d->v->fs->read_block(file->d, temp, blocknum);
            if (actual != bs)
                goto failure;
            actual = MIN(bs - offset % bs, length);
            memcpy(buffer, &temp[offset % bs], (u32) actual);
        } else if (length >= bs) {
            actual = file->d->v->fs->read_block(file->d, buffer, blocknum);
            if (actual != bs)
                goto failure;
        } else {
            actual = file->d->v->fs->read_block(file->d, temp, blocknum);
            if (actual != bs)
                goto failure;
            actual = length;
            memcpy(buffer, temp, (u32) actual);
        }

        buffer += actual;
        length -= actual;
        offset += actual;
        total += actual;
    }

    kfree(temp);
    return total;

    failure:
    kfree(temp);
    if (total == 0)
        return -1;
    return total;
}

int fs_dirent_mkdir(struct fs_dirent *d, const char *name) {
    if (!d->v->fs->mkdir)
        return 0;
    return d->v->fs->mkdir(d, name);
}

int fs_dirent_mkfile(struct fs_dirent *d, const char *name) {
    if (!d->v->fs->mkfile)
        return 0;
    return d->v->fs->mkfile(d, name);
}

int fs_dirent_rmdir(struct fs_dirent *d, const char *name) {
    if (!d->v->fs->rmdir)
        return 0;
    return d->v->fs->rmdir(d, name);
}

int fs_dirent_unlink(struct fs_dirent *d, const char *name) {
    if (!d->v->fs->unlink)
        return 0;
    return d->v->fs->unlink(d, name);
}

int fs_file_write(struct fs_file *file, const char *buffer, u32 length, u32 offset) {
    int total = 0;
    int bs = file->d->v->block_size;

    if (!file->d->v->fs->write_block || !file->d->v->fs->read_block)
        return -1;

    char *temp = kmalloc(4096, PAGESIZE, "fsFileWrite");

    while (length > 0) {

        int blocknum = offset / bs;
        int actual = 0;

        if (offset % bs) {
            actual = file->d->v->fs->read_block(file->d, temp, blocknum);
            if (actual != bs)
                goto failure;

            actual = MIN(bs - offset % bs, length);
            memcpy(&temp[offset % bs], buffer, (u32)actual);

            int wactual = file->d->v->fs->write_block(file->d, temp, blocknum);
            if (wactual != bs)
                goto failure;

        } else if (length >= bs) {
            actual = file->d->v->fs->write_block(file->d, buffer, blocknum);
            if (actual != bs)
                goto failure;
        } else {
            actual = file->d->v->fs->read_block(file->d, temp, blocknum);
            if (actual != bs)
                goto failure;

            actual = length;
            memcpy(temp, buffer, (u32) actual);

            int wactual = file->d->v->fs->write_block(file->d, temp, blocknum);
            if (wactual != bs)
                goto failure;
        }

        buffer += actual;
        length -= actual;
        offset += actual;
        total += actual;
    }

    kfree(temp);
    return total;

    failure:
    kfree(temp);
    if (total == 0)
        return -1;
    return total;
}

int fs_file_get_dimensions(struct fs_file *f, int *dims, int n) {
    if (n <= 0)
        return 0;

    dims[0] = f->size;

    return 1;
}
