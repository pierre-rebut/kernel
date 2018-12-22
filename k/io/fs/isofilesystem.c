//
// Created by rebut_p on 22/12/18.
//

#include <sys/allocator.h>
#include <stdio.h>
#include <string.h>
#include <sys/filesystem.h>
#include <io/ata.h>

#include "isofilesystem.h"
#include "iso.h"

#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
//#define LOG(x, ...)

struct iso_volume {
    int unit;
    int root_sector;
    int root_length;
    int total_sectors;
};

struct iso_dirent {
    struct iso_volume *volume;
    int sector;
    int length;
    int isdir;
    u32 offset;
};

static struct FsVolume *iso_volume_as_volume(struct iso_volume *cdv);

static struct FsPath *iso_dirent_as_dirent(struct iso_dirent *cdd);

int strcmp_iso_ident(const char *ident, const char *s);

static struct iso_dirent *iso_dirent_create(struct iso_volume *volume, int sector, int length, int isdir) {
    struct iso_dirent *d = kmalloc(sizeof(struct iso_dirent), 0, "newIsoDirent");
    if (!d)
        return 0;

    d->offset = 0;
    d->volume = volume;
    d->sector = sector;
    d->length = length;
    d->isdir = isdir;
    return d;
}

static char *iso_dirent_load(struct FsPath *d) {
    struct iso_dirent *cdd = d->privateData;
    int nsectors = cdd->length / ATAPI_BLOCKSIZE + (cdd->length % ATAPI_BLOCKSIZE) ? 1 : 0;
    char *data = kmalloc(nsectors * ATAPI_BLOCKSIZE, 0, "isoAtapiSector");
    if (!data)
        return 0;

    atapi_read(cdd->volume->unit, data, nsectors, cdd->sector);
    // XXX check result

    return data;
}

static void fix_filename(char *name, int length) {
    // Plain files typically end with a semicolon and version, remove it.
    if (length > 2 && name[length - 2] == ';') {
        length -= 2;
    }
    // Files without a suffix end with a dot, remove it.
    if (length > 1 && name[length - 1] == '.') {
        length--;
    }
    // In any case, null-terminate the string
    name[length] = 0;
}

static int iso_dirent_read_block(struct FsPath *d, char *buffer, u32 blocknum) {
    struct iso_dirent *cdd = d->privateData;
    int nblocks = atapi_read(cdd->volume->unit, buffer, 1, cdd->sector + blocknum);
    if (nblocks == 1) {
        return ATAPI_BLOCKSIZE;
    } else {
        return -1;
    }
}

static struct FsPath *iso_dirent_lookup(struct FsPath *dir, const char *name) {
    struct iso_dirent *cddir = dir->privateData;
    char *data = iso_dirent_load(dir);
    if (!data)
        return 0;

    int data_length = cddir->length;

    struct iso_9660_directory_entry *d = (struct iso_9660_directory_entry *) data;
    char *upper_name = strdup(name);
    if (!upper_name) {
        kfree(data);
        return NULL;
    }
    strtoupper(upper_name);

    while (data_length > 0 && d->descriptor_length > 0) {
        fix_filename(d->ident, d->ident_length);

        if (!strcmp_iso_ident(d->ident, upper_name)) {
            struct iso_dirent *r = iso_dirent_create(cddir->volume,
                                                     d->first_sector_little,
                                                     d->length_little,
                                                     d->flags & ISO_9660_EXTENT_FLAG_DIRECTORY);

            kfree(data);
            kfree(upper_name);
            return iso_dirent_as_dirent(r);
        }

        d = (struct iso_9660_directory_entry *) ((char *) d + d->descriptor_length);
        data_length -= d->descriptor_length;
    }

    kfree(data);
    kfree(upper_name);

    return 0;
}

static struct dirent *iso_dirent_read_dir(struct FsPath *dir, struct dirent *result) {
    struct iso_dirent *cddir = dir->privateData;

    if (!cddir->isdir)
        return NULL;

    char *data = iso_dirent_load(dir);
    if (!data)
        return NULL;

    int data_length = cddir->length;

    struct iso_9660_directory_entry *d = (struct iso_9660_directory_entry *) data;
    u32 offset = 0;

    while (offset < cddir->offset && data_length > 0 && d->descriptor_length > 0) {
        d = (struct iso_9660_directory_entry *) ((char *) d + d->descriptor_length);
        data_length -= d->descriptor_length;
        offset++;
    }

    if (offset != cddir->offset)
        return NULL;

    fix_filename(d->ident, d->ident_length);

    result->d_ino = offset;

    if (d->ident[0] == 0) {
        result->d_type = FT_DIRECTORY;
        strcpy(result->d_name, ".");
    } else if (d->ident[0] == 1) {
        result->d_type = FT_DIRECTORY;
        strcpy(result->d_name, "..");
    } else {
        strcpy(result->d_name, d->ident);
        result->d_type = FT_FILE;
    }

    kfree(data);
    return result;
}

int strcmp_iso_ident(const char *ident, const char *s) {
    if (ident[0] == 0 && (strcmp(s, ".") == 0)) {
        return 0;
    }
    if (ident[0] == 1 && (strcmp(s, "..") == 0)) {
        return 0;
    }
    return strcmp(ident, s);
}

static int iso_dirent_close(struct FsPath *d) {
    struct iso_dirent *cdd = d->privateData;
    kfree(cdd);
    kfree(d);
    return 0;
}

static struct FsPath *iso_volume_root(struct FsVolume *v) {
    struct iso_volume *cdv = v->privateData;
    struct iso_dirent *cdd = iso_dirent_create(cdv, cdv->root_sector, cdv->root_length, 1);
    return iso_dirent_as_dirent(cdd);
}

static struct FsVolume *iso_volume_open(void *data) {
    kSerialPrintf("%p\n", data);
    int unit = (int)data;

    struct iso_volume *cdv = kmalloc(sizeof(struct iso_volume), 0, "newIsoVolume");
    if (!cdv)
        return 0;

    struct iso_9660_volume_descriptor *d = kmalloc(ATAPI_BLOCKSIZE, 0, "iso");
    if (!d) {
        kfree(cdv);
        return 0;
    }

    LOG("isofs: scanning atapi unit %d...\n", unit);

    int j;

    for (j = 0; j < 16; j++) {
        LOG("isofs: checking volume %d\n", j);

        atapi_read(unit, d, 1, j + 16);
        // XXX check reuslt

        if (strncmp(d->magic, "CD001", 5))
            continue;

        if (d->type == ISO_9660_VOLUME_TYPE_PRIMARY) {
            cdv->root_sector = d->root.first_sector_little;
            cdv->root_length = d->root.length_little;
            cdv->total_sectors = d->nsectors_little;
            cdv->unit = unit;

            LOG("isofs: mounted filesystem on unit %d\n", cdv->unit);

            kfree(d);
            return iso_volume_as_volume(cdv);

        } else if (d->type == ISO_9660_VOLUME_TYPE_TERMINATOR) {
            break;
        } else {
            continue;
        }
    }

    kfree(d);
    LOG("isofs: no filesystem found\n");
    return 0;
}

static int iso_volume_close(struct FsVolume *v) {
    struct iso_volume *cdv = v->privateData;
    LOG("isofs: umounted filesystem from unit %d\n", cdv->unit);
    kfree(v);
    return 0;
}

static struct FsVolume *iso_volume_as_volume(struct iso_volume *cdv) {
    struct FsVolume *v = kmalloc(sizeof(struct FsVolume), 0, "convIsoVolToFsVol");
    if (!v)
        return NULL;

    v->privateData = cdv;
    v->blockSize = ATAPI_BLOCKSIZE;
    return v;
}

static struct FsPath *iso_dirent_as_dirent(struct iso_dirent *cdd) {
    struct FsPath *d = kmalloc(sizeof(struct FsPath), 0, "convIsoDirToFsPath");
    if (!d)
        return NULL;

    d->privateData = cdd;
    d->size = (u32) cdd->length;
    return d;
}

static struct Fs fs_iso = {
        "iso",
        .mount = &iso_volume_open,
        .umount = &iso_volume_close,
        .root = &iso_volume_root,
        .close = &iso_dirent_close,
        .readdir = &iso_dirent_read_dir,
        .lookup = &iso_dirent_lookup,
        .readBlock = &iso_dirent_read_block
};

void initIsoFileSystem() {
    fsRegister(&fs_iso);
}