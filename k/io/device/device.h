//
// Created by rebut_p on 24/12/18.
//

#ifndef KERNEL_DEVICE_H
#define KERNEL_DEVICE_H

#include <k/ktypes.h>

struct DeviceDriver
{
    char *name;
    u32 multiplier;

    void (*reset)(int id);

    int (*read)(int id, void *buffer, int nblocks, int offset);

    int (*write)(int id, const void *buffer, int nblocks, int offset);

    int (*probe)(int id, unsigned int *nblocks, int *blocksize, char *name);

    struct DeviceDriver *next;
};

struct Device
{
    char *name;
    int unit;
    int blockSize;
    u32 nblocks;
    int multiplier;
    struct DeviceDriver *driver;
};

void deviceDriverRegister(struct DeviceDriver *driver);

struct Device *deviceCreate(const char *name, struct DeviceDriver *driver, int unit, u32 nblocks, int blockSize);

struct Device *deviceGetByName(const char *name);

void deviceDestroy(struct Device *device);

void deviceReset(struct Device *device);

int deviceRead(struct Device *device, void *buffer, int size, int offset);

int deviceWrite(struct Device *device, const void *buffer, int size, int offset);

struct DeviceDriver *deviceGetDeviceDriverByIndex(u32 index);

struct DeviceDriver *deviceGetDeviceDriverByName(const char *name);

struct Device *deviceGetByIndex(u32 index);

inline int deviceBlockSize(struct Device *d)
{
    return d->blockSize * d->multiplier;
}

inline int deviceNblocks(struct Device *d)
{
    return d->nblocks / d->multiplier;
}

inline int deviceUnit(struct Device *d)
{
    return d->unit;
}

inline const char *deviceDriverName(struct Device *d)
{
    return d->driver->name;
}

#endif //KERNEL_DEVICE_H
