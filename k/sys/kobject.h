//
// Created by rebut_p on 20/12/18.
//

#ifndef KERNEL_KOBJECT_H
#define KERNEL_KOBJECT_H

#include <kstd.h>

enum KObjectType {
    KO_FS_FILE,
    KO_FS_FOLDER,
    KO_CONS_STD,
    KO_PIPE,
    KO_CONS_ERROR ,
    KO_DEVICE,
    KO_PROC
};

struct Kobject {
    void *data;
    enum KObjectType type;
    u32 offset;
    int mode;

    u32 refcount;
};

struct Kobject *koCreate(enum KObjectType type, void *data, int mode);
s32 koRead(struct Kobject *kobject, void *buffer, u32 size);
s32 koWrite(struct Kobject *kobject, void *buffer, u32 size);
int koDestroy(struct Kobject *kobject);
off_t koSeek(struct Kobject *obj, off_t offset, int whence);
struct Kobject *koDupplicate(struct Kobject *obj);

#endif //KERNEL_KOBJECT_H
