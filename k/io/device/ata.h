//
// Created by rebut_p on 16/12/18.
//

#ifndef KERNEL_ATA_H
#define KERNEL_ATA_H

#define ATA_BLOCKSIZE 512
#define ATAPI_BLOCKSIZE 2048

struct ata_count {
	int blocks_written[4];
	int blocks_read[4];
};

void ata_init();

struct ata_count ata_stats();

#endif //KERNEL_ATA_H
