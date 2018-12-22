//
// Created by rebut_p on 20/12/18.
//

#ifndef KERNEL_KOBJECT_H
#define KERNEL_KOBJECT_H

#include "filesystem.h"
#include "console.h"

enum KObjectType {
    KO_FS,
    KO_FS_FOLDER,
    KO_CONS,
    KO_ERROR
};

struct Kobject {
    void *data;
    enum KObjectType type;
    u32 offset;
    int mode;
};

struct Kobject *koCreate(enum KObjectType type, void *data, int mode);
s32 koRead(struct Kobject *kobject, void *buffer, u32 size);
s32 koWrite(struct Kobject *kobject, void *buffer, u32 size);
int koDestroy(struct Kobject *kobject);
off_t koSeek(struct Kobject *obj, off_t offset, int whence);

#endif //KERNEL_KOBJECT_H
