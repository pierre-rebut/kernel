//
// Created by rebut_p on 16/12/18.
//

#include <kstdio.h>
#include <string.h>
#include <task.h>

#include <system/idt.h>
#include <system/mutex.h>
#include <io/pit.h>

#include "ata.h"
#include "io/io.h"
#include "io/pic.h"
#include "device.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define ATA_IRQ0    46
#define ATA_IRQ1    47
#define ATA_IRQ2    43
#define ATA_IRQ3    42

#define ATA_BASE0    0x1F0
#define ATA_BASE1    0x170
#define ATA_BASE2    0x1E8
#define ATA_BASE3    0x168

#define ATA_TIMEOUT 5000
#define ATA_IDENTIFY_TIMEOUT 1000

#define ATA_DATA    0    /* data register */
#define ATA_ERROR    1    /* error register */
#define ATA_COUNT    2    /* sectors to transfer */
#define ATA_SECTOR    3    /* sector number */
#define ATA_CYL_LO    4    /* low byte of cylinder number */
#define ATA_CYL_HI    5    /* high byte of cylinder number */
#define ATA_FDH        6    /* flags, drive and head */
#define ATA_STATUS    7
#define ATA_COMMAND    7
#define ATA_CONTROL    0x206
#define ATA_REG_ALTSTATUS  0x0C

#define ATA_FLAGS_ECC    0x80    /* enable error correction */
#define ATA_FLAGS_LBA    0x40    /* enable linear addressing */
#define ATA_FLAGS_SEC    0x20    /* enable 512-byte sectors */
#define ATA_FLAGS_SLV    0x10    /* address the slave drive */

#define ATA_STATUS_BSY    0x80    /* controller busy */
#define ATA_STATUS_RDY    0x40    /* drive ready */
#define ATA_STATUS_WF    0x20    /* write fault */
#define ATA_STATUS_SC    0x10    /* seek complete (obsolete) */
#define ATA_STATUS_DRQ    0x08    /* data transfer request */
#define ATA_STATUS_CRD    0x04    /* corrected data */
#define ATA_STATUS_IDX    0x02    /* index pulse */
#define ATA_STATUS_ERR    0x01    /* error */

#define ATA_COMMAND_IDLE        0x00
#define ATA_COMMAND_READ        0x20    /* read data */
#define ATA_COMMAND_WRITE        0x30    /* write data */
#define ATA_COMMAND_IDENTIFY        0xec

#define SCSI_READ10            0x28
#define SCSI_SENSE             0x03

#define ATA_CONTROL_RESET    0x04
#define ATA_CONTROL_DISABLEINT    0x02

static const int ata_base[4] = {ATA_BASE0, ATA_BASE0, ATA_BASE1, ATA_BASE1};


static int identify_in_progress = 0;

static struct ata_count counters = {0};

static struct Mutex ata_mutex;

static void ataInterruptHandler(struct esp_context *ctx)
{
    (void) ctx;
    klog("[ATA] interrupt\n");
}

static void ataReset(int id)
{
    outb(ata_base[id] + ATA_CONTROL, ATA_CONTROL_RESET);
    taskWaitEvent(TaskEventTimer, 1);
    outb(ata_base[id] + ATA_CONTROL, 0);
    taskWaitEvent(TaskEventTimer, 1);
}

static int ataWait(int id, int mask, int state)
{
    clock_t start, elapsed;

    LOG("[ata] wait\n");
    clock_t timeout_millis = identify_in_progress ? ATA_IDENTIFY_TIMEOUT : ATA_TIMEOUT;

    LOG("[ata] wait get start tick\n");
    start = gettick();

    for (int i = 0; i < 4; i++)
        inb(ata_base[id] + ATA_REG_ALTSTATUS);

    retry:;
    u8 status = inb(ata_base[id] + ATA_STATUS);
    if (status & ATA_STATUS_BSY)
        goto retry;

    retry2:
    status = inb(ata_base[id] + ATA_STATUS);
    if (status & ATA_STATUS_ERR) {
        klog("[ata] ERR set, device failure!\n");
        ataReset(id);
        return 0;
    }

    if ((status & mask) != state) {
        LOG("[ata] check time\n");
        elapsed = gettick() - start;
        if (elapsed > timeout_millis) {
            if (!identify_in_progress) {
                klog("[ata] wait timeout\n");
            }
            ataReset(id);
            LOG("[ata] wait end\n");
            return 0;
        }
        goto retry2;
    }
    return 1;
}

static void ataReadPio(int id, void *buffer, int size)
{
    u16 *wbuffer = (u16 *) buffer;
    while (size > 0) {
        *wbuffer = inw(ata_base[id] + ATA_DATA);
        wbuffer++;
        size -= 2;
    }
}

static void ataWritePio(int id, const void *buffer, int size)
{
    u16 *wbuffer = (u16 *) buffer;
    while (size > 0) {
        outw(ata_base[id] + ATA_DATA, *wbuffer);
        wbuffer++;
        size -= 2;
    }
}


static int ataBegin(int id, int command, int nblocks, int offset)
{
    int base = ata_base[id];
    int sector, clow, chigh, flags;

    flags = ATA_FLAGS_ECC | ATA_FLAGS_LBA | ATA_FLAGS_SEC;

    if (id % 2)
        flags |= ATA_FLAGS_SLV;

    sector = (offset >> 0) & 0xff;
    clow = (offset >> 8) & 0xff;
    chigh = (offset >> 16) & 0xff;
    flags |= (offset >> 24) & 0x0f;

    if (!ataWait(id, ATA_STATUS_BSY, 0))
        return 0;

    outb(base + ATA_FDH, flags);

    int ready = ataWait(id, ATA_STATUS_BSY | ATA_STATUS_RDY, ATA_STATUS_RDY);

    if (!ready)
        return 0;

    outb(base + ATA_CONTROL, 0);
    outb(base + ATA_COUNT, nblocks);
    outb(base + ATA_SECTOR, sector);
    outb(base + ATA_CYL_LO, clow);
    outb(base + ATA_CYL_HI, chigh);
    outb(base + ATA_FDH, flags);

    outb(base + ATA_COMMAND, command);

    return 1;
}

static int ataReadBlockUnlocked(int id, void *buffer, int nblocks, int offset)
{
    LOG("ata read unlocked\n");

    if (!ataBegin(id, ATA_COMMAND_READ, nblocks, offset))
        return -2;

    LOG("[ata] read unlocked: loop\n");

    for (int i = 0; i < nblocks; i++) {
        LOG("[ata] wait for data: %d\n", i);
        if (!ataWait(id, ATA_STATUS_DRQ, ATA_STATUS_DRQ))
            return -3;
        LOG("[ata] read nblock: %d\n", i);
        ataReadPio(id, buffer, ATA_BLOCKSIZE);
        buffer += ATA_BLOCKSIZE;
        LOG("[ata] read end: %d\n", i);
    }
    LOG("[ata] wait busy\n");
    if (!ataWait(id, ATA_STATUS_BSY, 0))
        return -4;
    LOG("[ata] read unlocked end\n");
    return nblocks;
}

static int ataReadBlock(int id, void *buffer, int nblocks, int offset)
{
    int result;
    mutexLock(&ata_mutex);
    LOG("[ata] read: %u\n", nblocks);
    result = ataReadBlockUnlocked(id, buffer, nblocks, offset);
    LOG("[ata] read: res = %d\n", result);
    mutexUnlock(&ata_mutex);
    counters.blocks_read[id] += nblocks;
    return result;
}

static int ataWriteUnlocked(int id, const void *buffer, int nblocks, int offset)
{
    LOG("ata write unlocked\n");

    int i;
    if (!ataBegin(id, ATA_COMMAND_WRITE, nblocks, offset))
        return 0;

    LOG("[ata] write unlocked: loop\n");

    for (i = 0; i < nblocks; i++) {
        LOG("[ata] wait for ready: %d\n", i);
        if (!ataWait(id, ATA_STATUS_DRQ, ATA_STATUS_DRQ))
            return 0;

        LOG("[ata] write nblock: %d\n", i);
        ataWritePio(id, buffer, ATA_BLOCKSIZE);
        buffer = ((char *) buffer) + ATA_BLOCKSIZE;
        offset++;
        LOG("[ata] write end: %d\n", i);
    }
    // XXX On fast virtual hardware, waiting for the interrupt
    // doesn't work b/c it has already arrived before we get here.
    // For now, busy wait until a fix is in place.

    // if(ata_interrupt_active) process_wait(&queue);

    LOG("[ata] wait busy\n");
    if (!ataWait(id, ATA_STATUS_BSY, 0))
        return 0;

    LOG("[ata] write unlocked end\n");
    return nblocks;
}

int ataWriteBlock(int id, const void *buffer, int nblocks, int offset)
{
    int result;
    mutexLock(&ata_mutex);
    LOG("[ata] write: %u\n", nblocks);
    result = ataWriteUnlocked(id, buffer, nblocks, offset);
    LOG("[ata] write: res = %d\n", result);
    mutexUnlock(&ata_mutex);
    counters.blocks_written[id] += nblocks;
    return result;
}

static int ataIdentify(int id, int command, void *buffer)
{
    int result = 0;
    identify_in_progress = 1;
    if (ataBegin(id, command, 0, 0)) {
        if (ataWait(id, ATA_STATUS_DRQ, ATA_STATUS_DRQ)) {
            ataReadPio(id, buffer, 512);
            result = 1;
        }
    }
    identify_in_progress = 0;
    return result;
}


static int ataProbe(int id, unsigned int *nblocks, int *blocksize, char *name)
{
    u16 buffer[256];
    char *cbuffer = (char *) buffer;

    u8 t = inb(ata_base[id] + ATA_STATUS);
    if (t == 0xff) {
        kprintf("ata unit %d: nothing attached\n", id);
        return 0;
    }

    ataReset(id);

    memset(cbuffer, 0, 512);

    if (ataIdentify(id, ATA_COMMAND_IDENTIFY, cbuffer) == 0) {
        kprintf("ata unit %d: not connected\n", id);
        return 0;
    }

    *nblocks = buffer[1] * buffer[3] * buffer[6];
    *blocksize = ATA_BLOCKSIZE;

    u32 i;
    for (i = 0; i < 512; i += 2) {
        t = cbuffer[i];
        cbuffer[i] = cbuffer[i + 1];
        cbuffer[i + 1] = t;
    }
    cbuffer[255] = 0;

    strcpy(name, &cbuffer[54]);
    name[40] = 0;

    return 1;
}

static struct DeviceDriver ataDriver = {
        .name = "ata",
        .reset = &ataReset,
        .read = &ataReadBlock,
        .write = &ataWriteBlock,
        .probe = &ataProbe
};

void ataInit()
{
    for (int i = 0; i < 4; i++) {
        counters.blocks_read[i] = 0;
        counters.blocks_written[i] = 0;
    }

    mutexReset(&ata_mutex);

    kprintf("[ata] setting up interrupts\n");

    interruptRegister(ATA_IRQ0, &ataInterruptHandler);
    allowIrq(ATA_IRQ0);

    interruptRegister(ATA_IRQ1, &ataInterruptHandler);
    allowIrq(ATA_IRQ1);

    deviceDriverRegister(&ataDriver);

    kprintf("Probe ATA device\n");
    LOG("[ata] Probe device\n");

    char devName[4] = "sda";

    for (u32 i = 0; i < 4; i++) {
        u32 nblocks = 0;
        int blocksize = 0;
        char longname[256];

        if (ataProbe(i, &nblocks, &blocksize, longname) == 1) {
            kprintf("[ata] device found on %d: %s\n", i, longname);
            void *dev = deviceCreate(devName, &ataDriver, i, nblocks, blocksize);
            if (dev == NULL)
                continue;

            devName[3]++;
        }
    }
}