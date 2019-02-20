//
// Created by rebut_p on 16/12/18.
//

#ifndef KERNEL_ATA_H
#define KERNEL_ATA_H

#define ATA_BLOCKSIZE 512

struct ata_count
{
    int blocks_written[4];
    int blocks_read[4];
};

void ataInit();

#endif //KERNEL_ATA_H
