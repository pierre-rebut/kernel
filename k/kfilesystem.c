//
// Created by rebut_p on 23/09/18.
//

#include "kfilesystem.h"

#include <stdio.h>

static struct kfs_superblock *kfs = NULL;
static struct file_entry fdTable[256] = {0};

void initKFileSystem(module_t *module) {
    struct kfs_superblock *tmp = (struct kfs_superblock *) module->mod_start;

    if (tmp->magic != KFS_MAGIC) {
        printf("KFS - Bad magic number\n");
        return;
    }

    if (kfs_checksum(tmp, sizeof(struct kfs_superblock) - 4) != tmp->cksum) {
        printf("KFS - Bad checksum\n");
        return;
    }

    printf("KFS - init\n");
    kfs = tmp;
}

static int strcmp(const char *a, const char *b) {
    u32 i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i])
            return -1;

        i++;
    }
    if (a[i] != b[i])
        return -1;
    return 0;
}

static char checkBlockChecksum(struct kfs_block *block) {
    u32 cksum = block->cksum;
    block->cksum = 0;
    u32 tmp = kfs_checksum(block, sizeof(struct kfs_block));
    block->cksum = cksum;

    return tmp == cksum;
}

static char changeBlock(struct file_entry *file) {
    file->block = ((void *) kfs) + (file->d_blks[file->blockIndex] * KFS_BLK_SZ);

    if (checkBlockChecksum(file->block) == 0) {
        printf("KFS read - Bad checksum\n");
        return -1;
    }

    file->blockIndex++;
    return 0;
}

static struct kfs_inode *getFileINode(const char *pathname) {
    struct kfs_inode *node = (struct kfs_inode *) ((void *) kfs + (kfs->inode_idx * KFS_BLK_SZ));

    u32 i;
    for (i = 0; i < kfs->inode_cnt; i++) {
        if (strcmp(node->filename, pathname) == 0)
            break;
        node = (struct kfs_inode *) ((void *) kfs + (node->next_inode * KFS_BLK_SZ));
    }

    if (i == kfs->inode_cnt)
        return NULL;

    if (kfs_checksum(node, sizeof(struct kfs_inode) - 4) != node->cksum)
        return NULL;

    return node;
}

int open(const char *pathname, int flags) {
    if (kfs == NULL || pathname == NULL || flags != O_RDONLY)
        return -1;

    if (pathname[0] == '/')
        pathname++;

    struct kfs_inode *node = getFileINode(pathname);
    if (node == NULL)
        return -2;

    for (int i = 0; i < 256; i++) {
        if (fdTable[i].used == 0) {
            fdTable[i].blockIndex = 0;
            fdTable[i].d_blks = node->d_blks;
            if (changeBlock(&fdTable[i]) == -1) {
                return -4;
            }
            fdTable[i].used = 1;
            fdTable[i].node = node;
            fdTable[i].iblock = NULL;
            fdTable[i].iblockIndex = 0;
            fdTable[i].flags = flags;
            fdTable[i].dataIndex = 0;
            return i;
        }
    }
    return -5;
}

static char changeIBlock(struct file_entry *file, struct kfs_inode *node) {
    if (file->iblockIndex >= node->i_blk_cnt) {
        printf("KFS end of file\n");
        return -1;
    }

    file->iblock = ((void *) kfs) + (node->i_blks[file->iblockIndex] * KFS_BLK_SZ);
    if (kfs_checksum(file->iblock, sizeof(struct kfs_iblock) - 4) != file->iblock->cksum) {
        printf("KFS iblock - Bad checksum\n");
        return -1;
    }

    file->d_blks = file->iblock->blks;
    file->blockIndex = 0;
    file->iblockIndex++;

    return 0;
}

ssize_t read(int fd, void *buf, u32 size) {
    if (fd < 0 || fd > 255 || kfs == NULL)
        return -1;

    struct file_entry *file = &(fdTable[fd]);
    if (file->used == 0)
        return -1;

    struct kfs_inode *node = file->node;
    u8 *buffer = (u8 *) buf;

    for (u32 i = 0; i < size; i++) {
        if (file->dataIndex >= file->block->usage) {
            if ((file->iblock == NULL && file->blockIndex >= node->d_blk_cnt) ||
                (file->iblock != NULL && file->blockIndex >= file->iblock->blk_cnt)) {
                char res = changeIBlock(file, node);
                if (res == -1)
                    return i > 0 ? (ssize_t) i : -1;
            }
            if (changeBlock(file) == -1)
                return i > 0 ? (ssize_t) i : -1;
            file->dataIndex = 0;
        }

        buffer[i] = file->block->data[file->dataIndex];
        file->dataIndex++;
    }

    return (ssize_t) size;
}

static char seekSet(struct file_entry *file, off_t offset) {
    u32 blockId = (u32) offset / KFS_BLK_DATA_SZ;
    if (blockId < KFS_DIRECT_BLK) {
        file->iblock = NULL;
        file->iblockIndex = 0;
        file->blockIndex = (u32) blockId;
        file->d_blks = file->node->d_blks;
        if (changeBlock(file) == -1)
            return -1;
    } else {
        blockId -= KFS_DIRECT_BLK;
        file->iblockIndex = blockId / KFS_INDIRECT_BLK_CNT;

        if (changeIBlock(file, file->node) == -1)
            return -1;

        file->blockIndex = blockId % KFS_INDIRECT_BLK_CNT;
        if (changeBlock(file) == -1)
            return -1;
    }

    file->dataIndex = (u32) (offset % KFS_BLK_DATA_SZ);
    return 0;
}

off_t getOffset(struct file_entry *file) {
    off_t res = 0;

    if (file->iblock != NULL) {
        res += KFS_BLK_DATA_SZ * KFS_DIRECT_BLK;
        res += (file->iblockIndex - 1) * KFS_INDIRECT_BLK_CNT * KFS_BLK_DATA_SZ;
    }

    res += (file->blockIndex - 1) * KFS_BLK_DATA_SZ;
    res += file->dataIndex;

    return res;
}

off_t seek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd > 255 || kfs == NULL)
        return -1;

    struct file_entry *file = &(fdTable[fd]);
    if (file->used == 0 || offset >= (off_t) file->node->file_sz)
        return -1;

    if (whence != SEEK_CUR) {
        if (offset < 0) {
            offset = -offset;
            whence = whence == SEEK_SET ? SEEK_END : SEEK_SET;
        }

        if (whence == SEEK_END)
            offset = (off_t) file->node->file_sz - offset;

        char res = seekSet(file, offset);
        if (res == -1)
            return -1;
        return offset;
    }

    off_t currentOffset = getOffset(file);
    if (currentOffset == -1)
        return -1;

    offset = (currentOffset + offset) % file->node->file_sz;
    return seek(fd, offset, offset < 0 ? SEEK_END : SEEK_SET);
}

int close(int fd) {
    if (fd < 0 || fd > 255 || kfs == NULL)
        return -1;

    if (fdTable[fd].used == 0)
        return -2;

    fdTable[fd].used = 0;
    return 0;
}

void listFiles() {
    if (kfs == NULL)
        return;

    printf("KFS - Files: (%d)\n", kfs->inode_cnt);

    struct kfs_inode *node = (struct kfs_inode *) ((void *) kfs + (kfs->inode_idx * KFS_BLK_SZ));
    for (u32 i = 0; i < kfs->inode_cnt; i++) {
        if (kfs_checksum(node, sizeof(struct kfs_inode) - 4) != node->cksum) {
            printf("KFS node - Bad checksum\n");
            kfs = NULL;
            return;
        }

        printf("%d - file: %s, size: %d (cnt: %d, db_cnt: %d, ib_cnt: %d)\n", node->inumber, node->filename,
               node->file_sz, node->blk_cnt, node->d_blk_cnt, node->i_blk_cnt);
        node = (struct kfs_inode *) ((void *) kfs + (node->next_inode * KFS_BLK_SZ));
    }
}

u32 length(const char *pathname) {
    if (kfs == NULL || pathname == NULL)
        return 0;

    if (pathname[0] == '/')
        pathname++;

    struct kfs_inode *node = getFileINode(pathname);
    if (node == NULL)
        return 0;

    return node->file_sz;
}