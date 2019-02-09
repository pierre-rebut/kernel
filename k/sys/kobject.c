//
// Created by rebut_p on 20/12/18.
//

#include <io/terminal.h>
#include <io/serial.h>
#include <io/pipe.h>
#include "kobject.h"
#include "allocator.h"
#include "console.h"

struct Kobject *koCreate(enum KObjectType type, void *data, int mode) {
    struct Kobject *obj = kmalloc(sizeof(struct Kobject), 0, "newKobject");
    if (!obj)
        return NULL;

    obj->data = data;
    obj->type = type;
    obj->mode = mode;
    obj->offset = 0;
    obj->refcount = 1;
    return obj;
}

s32 koRead(struct Kobject *obj, void *buffer, u32 size) {
    if (obj->mode == O_WRONLY)
        return -1;

    int actual;

    switch (obj->type) {
        case KO_FS_FILE:
            actual = fsReadFile(obj->data, (char *) buffer, size, obj->offset);
            break;
        case KO_CONS_STD:
            return consoleReadKeyboard(obj->data, buffer, size);
        case KO_PIPE:
            actual = pipeRead(obj->data, buffer, size);
            break;
        default:
            return -1;
    }

    if (actual > 0)
        obj->offset += actual;

    return actual;
}

s32 koWrite(struct Kobject *obj, void *buffer, u32 size) {
    if (obj->mode == O_RDONLY)
        return -1;

    int actual;

    switch (obj->type) {
        case KO_FS_FILE:
            actual = fsReadFile(obj->data, (char *) buffer, size, obj->offset);
            break;
        case KO_CONS_STD:
            return consoleWriteStandard(obj->data, buffer, size);
        case KO_CONS_ERROR:
            return writeSerial(buffer, size);
        case KO_PIPE:
            actual = pipeWrite(obj->data, buffer, size);
            break;
        default:
            return -1;
    }

    if (actual > 0)
        obj->offset += actual;
    return actual;
}

struct Kobject *koDupplicate(struct Kobject *obj) {
    obj->refcount += 1;
    return obj;
}

int koDestroy(struct Kobject *obj) {
    obj->refcount -= 1;

    if (obj->refcount == 0) {
        switch (obj->type) {
            case KO_FS_FOLDER:
            case KO_FS_FILE:
                fsPathDestroy(obj->data);
                break;
            case KO_PIPE:
                pipeDelete(obj->data);
                break;
            default:;
        }

        kfree(obj);
    }
    return 0;
}

off_t koSeek(struct Kobject *obj, off_t offset, int whence) {
    if (obj->type != KO_FS_FILE)
        return -1;

    u32 fileSize = ((struct FsPath*)obj->data)->size;
    if (fileSize == 0)
        return 0;

    switch (whence) {
        case SEEK_SET:
            if (offset < 0)
                return -1;

            if ((u32) offset > fileSize)
                offset = fileSize;

            obj->offset = (u32) offset;
            break;
        case SEEK_END:
            if (offset < 0)
                return -1;

            if ((u32) offset > fileSize)
                obj->offset = 0;
            else
                obj->offset = fileSize - (u32) offset;
            break;
        case SEEK_CUR:
            if (obj->offset + offset > fileSize)
                obj->offset = fileSize;
            else
                obj->offset += (u32) offset;
            break;
        default:
            return -1;
    }

    return obj->offset;
}