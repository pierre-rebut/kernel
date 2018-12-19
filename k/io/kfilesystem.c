//
// Created by rebut_p on 23/09/18.
//

#include "kfilesystem.h"
#include "sys/filesystem.h"

#include <k/kfs.h>
#include <k/kstd.h>
#include <stdio.h>
#include <sys/allocator.h>
#include <string.h>

#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
//#define LOG(x, ...)

static char checkBlockChecksum(struct kfs_block *block) {
    u32 cksum = block->cksum;
    block->cksum = 0;
    u32 tmp = kfs_checksum(block, sizeof(struct kfs_block));
    block->cksum = cksum;

    return tmp == cksum;
}

static struct kfs_inode *getFileINode(struct kfs_superblock *kfs, const char *pathname) {
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

static struct FsPath *kfsRoot(struct FsVolume *volume) {
    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "kfsRoot");
    if (!rootPath)
        return NULL;
    rootPath->privateData = volume->privateData;
    return rootPath;
}

static struct FsVolume *kfsMount(void *data) {
    struct kfs_superblock *tmp = (struct kfs_superblock *) data;

    if (tmp->magic != KFS_MAGIC) {
        kSerialPrintf("KFS - Bad magic number\n");
        return NULL;
    }

    if (kfs_checksum(tmp, sizeof(struct kfs_superblock) - 4) != tmp->cksum) {
        kSerialPrintf("KFS - Bad checksum\n");
        return NULL;
    }

    struct FsVolume *kfsVolume = kmalloc(sizeof(struct FsVolume), 0, "newKfsVolume");
    if (kfsVolume == NULL)
        return NULL;

    kfsVolume->blockSize = KFS_BLK_SZ;
    kfsVolume->privateData = tmp;
    return kfsVolume;
}

static int kfsUmount(struct FsVolume *volume) {
    kfree(volume);
    return 0;
}

static int kfsClose(struct FsPath *path) {
    kfree(path);
    return 0;
}

static int kfsStat(struct FsPath *path, struct stat *result) {
    struct kfs_inode *inode = path->privateData;

    result->inumber = inode->inumber;
    result->file_sz = inode->file_sz;
    result->i_blk_cnt = inode->i_blk_cnt;
    result->d_blk_cnt = inode->d_blk_cnt;
    result->blk_cnt = inode->blk_cnt;
    result->idx = inode->idx;
    result->cksum = inode->cksum;
    return 0;
}

static struct FsPath *kfsLookup(struct FsPath *path, const char *name) {
    struct kfs_superblock *kfs = path->volume->privateData;
    struct kfs_inode *node = getFileINode(kfs, name);
    if (node == NULL)
        return NULL;

    struct FsPath *file = kmalloc(sizeof(struct FsPath), 0, "FsPath");
    if (!file)
        return NULL;

    file->privateData = node;
    file->size = node->file_sz;
    return file;
}

static struct dirent *kfsReaddir(struct FsPath *path, struct dirent *result) {
    struct kfs_inode *node = (struct kfs_inode*)path->privateData;
    struct kfs_superblock *kfs = path->volume->privateData;
    if (node->idx > kfs->inode_cnt)
        return NULL;

    strcpy(result->d_name, node->filename);
    result->d_ino = node->idx;
    result->d_type = FT_FILE;

    path->privateData = (struct kfs_inode *) (path->volume->privateData + (node->next_inode * KFS_BLK_SZ));
    return result;
}

static int kfsReadBlock(struct FsPath *path, char *buffer, u32 blocknum) {
    struct kfs_inode *node = (struct kfs_inode*)path->privateData;
    struct kfs_superblock *kfs = path->volume->privateData;

    u32 *blockIndex = node->d_blks;

    if (blocknum >= KFS_DIRECT_BLK) {
        u32 iblockIndex = (blocknum - KFS_DIRECT_BLK) / KFS_INDIRECT_BLK;
        if (iblockIndex >= node->i_blk_cnt) {
            kSerialPrintf("[KFS] bad inode index: %d (max %d)", iblockIndex, node->i_blk_cnt);
            return -1;
        }

        struct kfs_iblock *iblock = ((void *) kfs) + (node->i_blks[iblockIndex] * KFS_BLK_SZ);
        if (kfs_checksum(iblock, sizeof(struct kfs_iblock) - 4) != iblock->cksum) {
            kSerialPrintf("[KFS] iblock - Bad checksum\n");
            return -1;
        }

        blockIndex = iblock->blks;
        blocknum = (blocknum - KFS_DIRECT_BLK) % KFS_INDIRECT_BLK;
    }

    struct kfs_block *block = ((void *) kfs) + (blockIndex[blocknum] * KFS_BLK_SZ);
    if (checkBlockChecksum(block) == 0) {
        kSerialPrintf("[KFS] block - Bad checksum\n");
        return -1;
    }
    memcpy(buffer, block->data, block->usage);
    memset(buffer + block->usage, 0, KFS_BLK_SZ - block->usage);
    return KFS_BLK_SZ;
}

static struct Fs fs_kfs = {
        .name = "kfs",
        .mount = &kfsMount,
        .umount = &kfsUmount,
        .root = &kfsRoot,
        .lookup = &kfsLookup,
        .readdir = &kfsReaddir,
        .readBlock = &kfsReadBlock,
        .stat = &kfsStat,
        .close = &kfsClose
};

void initKFileSystem() {
    fsRegister(&fs_kfs);
}
