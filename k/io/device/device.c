//
// Created by rebut_p on 24/12/18.
//


#include <sys/allocator.h>
#include <string.h>
#include <kstdio.h>
#include <include/list.h>
#include "device.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static struct DeviceDriver *driverList = NULL;
static struct List deviceList = CREATE_LIST();

void deviceDriverRegister(struct DeviceDriver *driver) {
    driver->next = driverList;
    driverList = driver;
}

struct DeviceDriver *deviceGetDeviceDriverByIndex(u32 index) {
    struct DeviceDriver *tmpDriver = driverList;
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
    struct DeviceDriver *tmpDriver = driverList;

    while (tmpDriver != NULL) {
        if (strcmp(tmpDriver->name, name) == 0)
            return tmpDriver;

        tmpDriver = tmpDriver->next;
    }
    return NULL;
}

struct Device *deviceCreate(const char *name, struct DeviceDriver *driver, int unit, u32 nblocks, int blockSize) {
    LOG("Device blocksize = %d - %u\n", blockSize, nblocks);

    struct Device *device = kmalloc(sizeof(struct Device), 0, "newDevice");
    if (device == NULL)
        return NULL;

    device->name = strdup(name);
    device->unit = unit;
    device->driver = driver;
    device->blockSize = blockSize;
    device->nblocks = nblocks;

    if (driver->multiplier != 0)
        device->multiplier = driver->multiplier;
    else
        device->multiplier = 1;

    listAddElem(&deviceList, device);
    return device;
}

static int deviceGetFromList(void *data, va_list ap) {
    struct Device *device = data;
    const char *name = va_arg(ap, const char *);

    return strcmp(device->name, name) == 0 ? 1 : 0;
}

struct Device *deviceGetByName(const char *name) {
    return listGetElem(&deviceList, &deviceGetFromList, name);
}

void deviceDestroy(struct Device *device) {
    kfree(device->name);
    kfree(device);
    listDeleteElem(&deviceList, device);
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