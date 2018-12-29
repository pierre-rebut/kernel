//
// Created by rebut_p on 16/12/18.
//

#include <stdio.h>
#include <sys/idt.h>
#include <string.h>
#include <task.h>
#include <sys/mutex.h>
#include "ata.h"
#include "io/io.h"
#include "io/pic.h"
#include "io/pit.h"
#include "device.h"

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
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

#define ATAPI_COMMAND_IDENTIFY 0xa1
#define ATAPI_COMMAND_PACKET   0xa0

#define ATAPI_FEATURE    1
#define ATAPI_IRR 2
#define ATAPI_SAMTAG 3
#define ATAPI_COUNT_LO 4
#define ATAPI_COUNT_HI 5
#define ATAPI_DRIVE 6

#define SCSI_READ10            0x28
#define SCSI_SENSE             0x03

#define ATA_CONTROL_RESET    0x04
#define ATA_CONTROL_DISABLEINT    0x02

static const int ata_base[4] = {ATA_BASE0, ATA_BASE0, ATA_BASE1, ATA_BASE1};

static int ata_interrupt_active = 0;
//static struct list queue = {0, 0};

static int identify_in_progress = 0;

static struct ata_count counters = {0};

static struct Mutex ata_mutex;

struct ata_count ata_stats() {
    return counters;
}

static void ata_interrupt(struct esp_context *ctx) {
    (void) ctx;
    ata_interrupt_active = 1;
    LOG("[ATA] interrupt\n");
    // process_wakeup_all(&queue);
}

static void ata_reset(int id) {
    outb(ata_base[id] + ATA_CONTROL, ATA_CONTROL_RESET);
    taskWaitEvent(TaskEventTimer, 1);
    outb(ata_base[id] + ATA_CONTROL, 0);
    taskWaitEvent(TaskEventTimer, 1);
}

static int ata_wait(int id, int mask, int state) {
    clock_t start, elapsed;
    int t;

    LOG("[ata] wait\n");
    clock_t timeout_millis = identify_in_progress ? ATA_IDENTIFY_TIMEOUT : ATA_TIMEOUT;

    LOG("[ata] wait get start tick\n");
    start = gettick();

    LOG("[ata] wait loop\n");
    while (1) {
        LOG("[ata] inb data\n");
        t = inb(ata_base[id] + ATA_STATUS);
        if ((t & mask) == state) {
            LOG("[ata] return ok\n");
            return 1;
        }
        if (t & ATA_STATUS_ERR) {
            kSerialPrintf("ata: error\n");
            ata_reset(id);
            return 0;
        }
        LOG("[ata] check time\n");
        elapsed = gettick() - start;
        if (elapsed > timeout_millis) {
            if (!identify_in_progress) {
                kSerialPrintf("ata: timeout\n");
            }
            ata_reset(id);
            LOG("[ata] wait end\n");
            return 0;
        }
        LOG("[ata] wait event\n");
        taskWaitEvent(TaskEventTimer, 1);
        LOG("[ata] loop end\n");
    }
}

static void ata_pio_read(int id, void *buffer, int size) {
    u16 *wbuffer = (u16 *) buffer;
    while (size > 0) {
        *wbuffer = inw(ata_base[id] + ATA_DATA);
        wbuffer++;
        size -= 2;
    }
}

static void ata_pio_write(int id, const void *buffer, int size) {
    u16 *wbuffer = (u16 *) buffer;
    while (size > 0) {
        outw(ata_base[id] + ATA_DATA, *wbuffer);
        wbuffer++;
        size -= 2;
    }
}

static int ata_begin(int id, int command, int nblocks, int offset) {
    int base = ata_base[id];
    int sector, clow, chigh, flags;

    // enable error correction and linear addressing
    flags = ATA_FLAGS_ECC | ATA_FLAGS_LBA | ATA_FLAGS_SEC;

    // turn on the slave bit for odd-numbered drives
    if (id % 2)
        flags |= ATA_FLAGS_SLV;

    // slice up the linear address in order to fit in the arguments
    sector = (offset >> 0) & 0xff;
    clow = (offset >> 8) & 0xff;
    chigh = (offset >> 16) & 0xff;
    flags |= (offset >> 24) & 0x0f;

    // wait for the disk to calm down
    if (!ata_wait(id, ATA_STATUS_BSY, 0))
        return 0;

    // get the attention of the proper disk
    outb(base + ATA_FDH, flags);

    // wait again for the disk to indicate ready
    // special case: ATAPI identification does not raise RDY flag

    int ready;
    if (command == ATAPI_COMMAND_IDENTIFY) {
        ready = ata_wait(id, ATA_STATUS_BSY, 0);
    } else {
        ready = ata_wait(id, ATA_STATUS_BSY | ATA_STATUS_RDY, ATA_STATUS_RDY);
    }

    if (!ready)
        return 0;

    // send the arguments
    outb(base + ATA_CONTROL, 0);
    outb(base + ATA_COUNT, nblocks);
    outb(base + ATA_SECTOR, sector);
    outb(base + ATA_CYL_LO, clow);
    outb(base + ATA_CYL_HI, chigh);
    outb(base + ATA_FDH, flags);

    // execute the command
    outb(base + ATA_COMMAND, command);

    return 1;
}

static int ata_read_unlocked(int id, void *buffer, int nblocks, int offset) {
    LOG("ata read unlocked\n");

    if (!ata_begin(id, ATA_COMMAND_READ, nblocks, offset))
        return -2;

    // XXX On fast virtual hardware, waiting for the interrupt
    // doesn't work b/c it has already arrived before we get here.
    // For now, busy wait until a fix is in place.

    // if(ata_interrupt_active) process_wait(&queue);

    LOG("[ata] read unlocked: loop\n");

    for (int i = 0; i < nblocks; i++) {
        LOG("[ata] wait for data: %d\n", i);
        if (!ata_wait(id, ATA_STATUS_DRQ, ATA_STATUS_DRQ))
            return -3;
        LOG("[ata] read nblock: %d\n", i);
        ata_pio_read(id, buffer, ATA_BLOCKSIZE);
        buffer += ATA_BLOCKSIZE;
        LOG("[ata] read end: %d\n", i);
    }
    LOG("[ata] wait busy\n");
    if (!ata_wait(id, ATA_STATUS_BSY, 0))
        return -4;
    LOG("[ata] read unlocked end\n");
    return nblocks;
}

static int ata_read(int id, void *buffer, int nblocks, int offset) {
    int result;
    mutexLock(&ata_mutex);
    LOG("[ata] read: %u\n", nblocks);
    result = ata_read_unlocked(id, buffer, nblocks, offset);
    LOG("[ata] read: res = %d\n", result);
    mutexUnlock(&ata_mutex);
    counters.blocks_read[id] += nblocks;
    /*if (current) {
        current->stats.blocks_read += nblocks;
        current->stats.bytes_read += nblocks * ATA_BLOCKSIZE;
    }*/
    return result;
}

static int atapi_begin(int id, void *data, int length) {
    int base = ata_base[id];
    int flags;

    // enable error correction and linear addressing
    flags = ATA_FLAGS_ECC | ATA_FLAGS_LBA | ATA_FLAGS_SEC;

    // turn on the slave bit for odd-numbered drives
    if (id % 2)
        flags |= ATA_FLAGS_SLV;

    // wait for the disk to calm down
    if (!ata_wait(id, ATA_STATUS_BSY, 0))
        return 0;

    // get the attention of the proper disk
    outb(base + ATA_FDH, flags);

    // wait again for the disk to indicate ready
    if (!ata_wait(id, ATA_STATUS_BSY, 0))
        return 0;

    // send the arguments
    outb(base + ATAPI_FEATURE, 0);
    outb(base + ATAPI_IRR, 0);
    outb(base + ATAPI_SAMTAG, 0);
    outb(base + ATAPI_COUNT_LO, length & 0xff);
    outb(base + ATAPI_COUNT_HI, length >> 8);

    // execute the command
    outb(base + ATA_COMMAND, ATAPI_COMMAND_PACKET);

    // wait for ready
    if (!ata_wait(id, ATA_STATUS_BSY | ATA_STATUS_DRQ, ATA_STATUS_DRQ))
        return 0;

    // send the ATAPI packet
    ata_pio_write(id, data, length);

    return 1;
}

static int atapi_read_unlocked(int id, void *buffer, int nblocks, int offset) {
    u8 packet[12];
    int length = sizeof(packet);
    int i;

    packet[0] = SCSI_READ10;
    packet[1] = 0;
    packet[2] = offset >> 24;
    packet[3] = offset >> 16;
    packet[4] = offset >> 8;
    packet[5] = offset >> 0;
    packet[6] = 0;
    packet[7] = nblocks >> 8;
    packet[8] = nblocks >> 0;
    packet[9] = 0;
    packet[10] = 0;
    packet[11] = 0;

    if (!atapi_begin(id, packet, length))
        return 0;

    // XXX On fast virtual hardware, waiting for the interrupt
    // doesn't work b/c it has already arrived before we get here.
    // For now, busy wait until a fix is in place.

    // if(ata_interrupt_active) process_wait(&queue);

    for (i = 0; i < nblocks; i++) {
        if (!ata_wait(id, ATA_STATUS_DRQ, ATA_STATUS_DRQ))
            return 0;
        ata_pio_read(id, buffer, ATAPI_BLOCKSIZE);
        buffer = ((char *) buffer) + ATAPI_BLOCKSIZE;
        offset++;
    }

    return 1;
}

static int atapi_read(int id, void *buffer, int nblocks, int offset) {
    LOG("[ATAPI] read\n");
    int result;
    // mutex_lock(&ata_mutex);
    result = atapi_read_unlocked(id, buffer, nblocks, offset);
    LOG("[ATAPI] result: %d\n", result);
    // mutex_unlock(&ata_mutex);
    counters.blocks_read[id] += nblocks;
    /*if (current) {
        current->stats.blocks_read += nblocks;
        current->stats.bytes_read += nblocks * ATAPI_BLOCKSIZE;
    }*/
    return result;
}

static int ata_write_unlocked(int id, const void *buffer, int nblocks, int offset) {
    int i;
    if (!ata_begin(id, ATA_COMMAND_WRITE, nblocks, offset))
        return 0;
    for (i = 0; i < nblocks; i++) {
        if (!ata_wait(id, ATA_STATUS_DRQ, ATA_STATUS_DRQ))
            return 0;
        ata_pio_write(id, buffer, ATA_BLOCKSIZE);
        buffer = ((char *) buffer) + ATA_BLOCKSIZE;
        offset++;
    }
    // XXX On fast virtual hardware, waiting for the interrupt
    // doesn't work b/c it has already arrived before we get here.
    // For now, busy wait until a fix is in place.

    // if(ata_interrupt_active) process_wait(&queue);

    if (!ata_wait(id, ATA_STATUS_BSY, 0))
        return 0;
    return nblocks;
}

static int ata_write(int id, const void *buffer, int nblocks, int offset) {
    int result;
    // mutex_lock(&ata_mutex);
    result = ata_write_unlocked(id, buffer, nblocks, offset);
    // mutex_unlock(&ata_mutex);
    counters.blocks_written[id] += nblocks;
    /*if (current) {
        current->stats.blocks_written += nblocks;
        current->stats.bytes_written += nblocks * ATA_BLOCKSIZE;
    }*/
    return result;
}

/*
ata_probe sends an IDENTIFY DEVICE command to the device.
If a device is connected, it will respond with 512 bytes
of identifying data, described on page 48 of the ATA-3 standard.
If no response comes within the timeout window, we assume
the the device is simply not connected.
*/

static int ata_identify(int id, int command, void *buffer) {
    int result = 0;
    identify_in_progress = 1;
    if (ata_begin(id, command, 0, 0)) {
        if (ata_wait(id, ATA_STATUS_DRQ, ATA_STATUS_DRQ)) {
            ata_pio_read(id, buffer, 512);
            result = 1;
        }
    }
    identify_in_progress = 0;
    return result;
}


static int ata_probe(int id, unsigned int *nblocks, int *blocksize, char *name) {
    u16 buffer[256];
    char *cbuffer = (char *) buffer;

    /*
       First check for 0xff in the controller status register,
       which would indicate that there is nothing attached.
     */

    u8 t = inb(ata_base[id] + ATA_STATUS);
    if (t == 0xff) {
        kprintf("ata unit %d: nothing attached\n", id);
        return 0;
    }

    /* Now reset the unit to check for register signatures. */
    ata_reset(id);

    /* Clear the buffer to receive the identify data. */
    memset(cbuffer, 0, 512);

    if (ata_identify(id, ATA_COMMAND_IDENTIFY, cbuffer)) {

        *nblocks = buffer[1] * buffer[3] * buffer[6];
        kprintf("%d logical cylinders\n", buffer[1]);
        kprintf("%d logical heads\n", buffer[3]);
        kprintf("%d logical sectors/track\n", buffer[6]);
        *blocksize = ATA_BLOCKSIZE;

    } /*else if (ata_identify(id, ATAPI_COMMAND_IDENTIFY, cbuffer)) {

        // XXX use SCSI sense to get media size
        *nblocks = 337920;
        *blocksize = ATAPI_BLOCKSIZE;

    } */ else {
        kprintf("ata unit %d: not connected\n", id);
        return 0;
    }

    /* Now byte-swap the data so as the generate byte-ordered strings */
    u32 i;
    for (i = 0; i < 512; i += 2) {
        t = cbuffer[i];
        cbuffer[i] = cbuffer[i + 1];
        cbuffer[i + 1] = t;
    }
    cbuffer[255] = 0;

    /* Vendor supplied name is at byte 54 */
    strcpy(name, &cbuffer[54]);
    name[40] = 0;

    /* Get disk size in megabytes*/
    //u32 mbytes = (*nblocks) / KILO * (*blocksize) / KILO;
    return 1;
}

static struct DeviceDriver ataDriver = {
        .name = "ata",
        .reset = &ata_reset,
        .read = &ata_read,
        .write = &ata_write,
        .probe = &ata_probe
};

static struct DeviceDriver atapiDriver = {
        .name = "atapi",
        .reset = &ata_reset,
        .read = &atapi_read,
        .probe = &ata_probe,
        .multiplier = 0
};

void ata_init() {
    for (int i = 0; i < 4; i++) {
        counters.blocks_read[i] = 0;
        counters.blocks_written[i] = 0;
    }

    mutexReset(&ata_mutex);

    kprintf("ata: setting up interrupts\n");

    interruptRegister(ATA_IRQ0, &ata_interrupt);
    allowIrq(ATA_IRQ0);

    interruptRegister(ATA_IRQ1, &ata_interrupt);
    allowIrq(ATA_IRQ1);

    deviceRegister(&ataDriver);
    deviceRegister(&atapiDriver);
}