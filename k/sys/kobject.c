//
// Created by rebut_p on 20/12/18.
//

#include <io/terminal.h>
#include "kobject.h"
#include "allocator.h"

struct Kobject *koCreate(enum KObjectType type, void *data, int mode) {
    struct Kobject *obj = kmalloc(sizeof(struct Kobject), 0, "newKobject");
    if (!obj)
        return NULL;

    obj->data = data;
    obj->type = type;
    obj->mode = mode;
    obj->offset = 0;
    return obj;
}

s32 koRead(struct Kobject *obj, void *buffer, u32 size) {
    if (obj->mode == O_WRONLY)
        return -1;

    switch (obj->type) {
        case KO_FS: {
            int actual = fsReadFile(obj->data, (char *) buffer, size, obj->offset);
            if (actual > 0)
                obj->offset += actual;
            return actual;
        }
        case Ko_CONS:
            return readKeyboardFromConsole(obj->data, buffer, size);
        default:
            return -1;
    }
}

s32 koWrite(struct Kobject *obj, void *buffer, u32 size) {
    if (obj->mode == O_RDONLY)
        return -1;

    switch (obj->type) {
        case KO_FS: {
            int actual = fsReadFile(obj->data, (char *) buffer, size, obj->offset);
            if (actual > 0)
                obj->offset += actual;
            return actual;
        }
        case Ko_CONS:
            return writeStringTerminal(buffer, size);
        default:
            return -1;
    }
}

int koDestroy(struct Kobject *obj) {
    switch (obj->type) {
        case Ko_FS_FOLDER:
        case KO_FS:
            fsPathDestroy(obj->data);
            break;
        default:
            ;
    }

    kfree(obj);
    return 0;
}