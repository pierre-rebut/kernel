//
// Created by rebut_p on 24/12/18.
//


#include <sys/allocator.h>
#include <string.h>
#include "device.h"

static struct DeviceDriver *deviceList = NULL;

void deviceRegister(struct DeviceDriver *driver) {
    driver->next = deviceList;
    deviceList = driver;
}

struct DeviceDriver *deviceGetDeviceDriverByIndex(u32 index) {
    struct DeviceDriver *tmpDriver = deviceList;
    u32 i = 0;

    while (tmpDriver != NULL) {
        if (i == index)
            return tmpDriver;
        tmpDriver = tmpDriver->next;
        i++;
    }

    return NULL;
}

struct DeviceDriver *deviceGetDeviceDriverByName(const char *name) {
    struct DeviceDriver *tmpDriver = deviceList;

    while (tmpDriver != NULL) {
        if (strcmp(tmpDriver->name, name) == 0)
            return tmpDriver;

        tmpDriver = tmpDriver->next;
    }
    return NULL;
}

struct Device *deviceCreate(const char *deviceName, int arg) {
    struct DeviceDriver *deviceDriver = deviceGetDeviceDriverByName(deviceName);
    if (deviceDriver == NULL || deviceDriver->probe == NULL)
        return NULL;

    u32 nblocks;
    int blockSize;
    char info[64];

    if (deviceDriver->probe(arg, &nblocks, &blockSize, info) == 0)
        return NULL;

    struct Device *device = kmalloc(sizeof(struct Device), 0, "newDevice");
    if (device == NULL)
        return NULL;

    device->unit = arg;
    device->driver = deviceDriver;
    device->blockSize = blockSize;
    device->nblocks = nblocks;

    if (deviceDriver->multiplier != 0)
        device->multiplier = deviceDriver->multiplier;
    else
        device->multiplier = 1;

    return device;
}

void deviceDestroy(struct Device *device) {
    kfree(device);
}

int deviceRead(struct Device *device, void *buffer, int size, int offset) {
    if (device == NULL || device->driver->read == NULL)
        return -1;

    return device->driver->read(device->unit, buffer, size * device->multiplier, offset * device->multiplier);
}

int deviceWrite(struct Device *device, const void *buffer, int size, int offset) {
    if (device == NULL || device->driver->write == NULL)
        return -1;

    return device->driver->write(device->unit, buffer, size * device->multiplier, offset * device->multiplier);
}

void deviceReset(struct Device *device) {
    if (device == NULL || device->driver->reset == NULL)
        return;

    device->driver->reset(device->unit);
}