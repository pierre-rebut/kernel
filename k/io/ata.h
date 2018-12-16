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
void ata_reset(int unit);
int ata_probe(int unit, unsigned int *nblocks, int *blocksize, char *name);

int ata_read(int unit, void *buffer, int nblocks, int offset);
int ata_write(int unit, const void *buffer, int nblocks, int offset);
int atapi_read(int unit, void *buffer, int nblocks, int offset);

#endif //KERNEL_ATA_H
