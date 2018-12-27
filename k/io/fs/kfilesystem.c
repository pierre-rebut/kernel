//
// Created by rebut_p on 23/09/18.
//

#include "kfilesystem.h"
#include "filesystem.h"

#include <k/kfs.h>
#include <stdio.h>
#include <sys/allocator.h>
#include <string.h>
#include <sys/paging.h>
#include <include/cpu.h>
#include <sys/physical-memory.h>

#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
//#define LOG(x, ...)

#define KFS_MEM_POS 0x1400000

static char checkBlockChecksum(struct kfs_block *block) {
    u32 cksum = block->cksum;
    block->cksum = 0;
    u32 tmp = kfs_checksum(block, sizeof(struct kfs_block));
    block->cksum = cksum;

    return tmp == cksum;
}

static struct kfs_inode *getFileINode(const char *pathname) {
    struct kfs_superblock *kfs = (struct kfs_superblock *) KFS_MEM_POS;
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

    struct kfs_superblock *kfs = volume->privateData;
    rootPath->privateData = volume->privateData + (kfs->inode_idx * KFS_BLK_SZ);
    return rootPath;
}

static struct FsVolume *kfsMount(u32 data) {
    LOG("kfs mount: %s\n", (const char *) data);

    void *pd = NULL;
    void *tmpData = NULL;

    struct FsPath *file = fsResolvePath((const char *) data);
    if (!file) {
        kSerialPrintf("[kfs] Can not open file: %s\n", (const char *) data);
        return NULL;
    }

    LOG("[kfs] get file stat\n");
    struct stat fileStat;
    if (fsStat(file, &fileStat) == -1) {
        kSerialPrintf("[kfs] Can not get file info: %s\n", (const char *) data);
        goto failure;
    }

    LOG("[kfs] alloc tmp data\n");
    tmpData = kmalloc(4096, 0, "kfsTmp");
    if (tmpData == NULL)
        goto failure;

    LOG("[kfs] create page directory\n");
    pd = pagingCreatePageDirectory();
    if (!pd)
        goto failure;

    u32 lengthFile = 0;
    LOG("[kfs] alloc page (addr: %X, size: %u)\n", KFS_MEM_POS, alignUp(fileStat.file_sz, PAGESIZE));
    if (pagingAlloc(pd, (void *) KFS_MEM_POS, alignUp(fileStat.file_sz, PAGESIZE), MEM_WRITE))
        goto failure;

    LOG("[kfs) read file and put into page alloc\n");
    while (lengthFile < fileStat.file_sz) {
        LOG("[kfs] read data from file\n");
        int tmp = fsReadFile(file, tmpData, 4096, lengthFile);
        LOG("[kfs] result: %d\n", tmp);
        if (tmp < 0)
            goto failure;
        if (tmp == 0)
            break;

        LOG("[kfs] memcpy to page alloc\n");
        cli();
        pagingSwitchPageDirectory(pd);
        memcpy((void *) KFS_MEM_POS + lengthFile, tmpData, (u32) tmp);
        pagingSwitchPageDirectory(currentTask->pageDirectory);
        sti();
        lengthFile += tmp;
        LOG("[kfs] current pos = %u\n", lengthFile);
    }

    LOG("[kfs] check kfs superblock\n");
    struct kfs_superblock *tmp = (struct kfs_superblock *) KFS_MEM_POS;

    cli();
    pagingSwitchPageDirectory(pd);
    int res = (tmp->magic != KFS_MAGIC || kfs_checksum(tmp, sizeof(struct kfs_superblock) - 4) != tmp->cksum);
    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();

    if (res) {
        kSerialPrintf("KFS - Bad superblock\n");
        goto failure;
    }

    LOG("[kfs] create FsVolume\n");
    struct FsVolume *kfsVolume = kmalloc(sizeof(struct FsVolume), 0, "newKfsVolume");
    if (kfsVolume == NULL)
        goto failure;

    kfsVolume->blockSize = KFS_BLK_DATA_SZ;
    kfsVolume->privateData = pd;
    kfree(tmpData);
    fsPathDestroy(file);
    return kfsVolume;

    failure:
    fsPathDestroy(file);
    kfree(tmpData);
    pagingDestroyPageDirectory(pd);
    return NULL;
}

static int kfsUmount(struct FsVolume *volume) {
    pagingDestroyPageDirectory(volume->privateData);
    kfree(volume);
    return 0;
}

static int kfsClose(struct FsPath *path) {
    kfree(path);
    return 0;
}

static int kfsStat(struct FsPath *path, struct stat *result) {
    struct kfs_inode *inode = path->privateData;

    cli();
    pagingSwitchPageDirectory(path->volume->privateData);
    result->inumber = inode->inumber;
    result->file_sz = inode->file_sz;
    result->i_blk_cnt = inode->i_blk_cnt;
    result->d_blk_cnt = inode->d_blk_cnt;
    result->blk_cnt = inode->blk_cnt;
    result->idx = inode->idx;
    result->cksum = inode->cksum;
    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();
    return 0;
}

static struct FsPath *kfsLookup(struct FsPath *path, const char *name) {
    LOG("[KFS] lookup fct\n");
    struct kfs_inode *node;

    if (*name == 0 || strcmp(name, ".") == 0)
        node = path->privateData;
    else {
        cli();
        pagingSwitchPageDirectory(path->volume->privateData);
        node = getFileINode(name);
        pagingSwitchPageDirectory(currentTask->pageDirectory);
        sti();
    }

    LOG("[KFS] lookup enter\n");
    if (node == NULL)
        return NULL;

    struct FsPath *file = kmalloc(sizeof(struct FsPath), 0, "FsPath");
    if (!file)
        return NULL;

    file->privateData = node;
    cli();
    pagingSwitchPageDirectory(path->volume->privateData);
    file->size = node->file_sz;
    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();
    return file;
}

static struct dirent *kfsReaddir(struct FsPath *path, struct dirent *result) {
    struct kfs_inode *node = (struct kfs_inode *) path->privateData;
    struct kfs_superblock *kfs = (struct kfs_superblock *) KFS_MEM_POS;

    cli();
    pagingSwitchPageDirectory(path->volume->privateData);

    LOG("[KFS] readdir: %s\n", node->filename);
    if (node->idx > kfs->inode_cnt) {
        pagingSwitchPageDirectory(currentTask->pageDirectory);
        sti();
        return NULL;
    }

    strcpy(result->d_name, node->filename);
    result->d_ino = node->idx;
    result->d_type = FT_FILE;

    path->privateData = (struct kfs_inode *) (path->volume->privateData + (node->next_inode * KFS_BLK_SZ));

    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();
    return result;
}

static int kfsReadBlock(struct FsPath *path, char *buffer, u32 blocknum) {
    struct kfs_inode *node = (struct kfs_inode *) path->privateData;
    struct kfs_superblock *kfs = (struct kfs_superblock *) KFS_MEM_POS;

    cli();
    pagingSwitchPageDirectory(path->volume->privateData);

    u32 *blockIndex = node->d_blks;

    if (blocknum >= KFS_DIRECT_BLK) {
        u32 iblockIndex = (blocknum - KFS_DIRECT_BLK) / KFS_INDIRECT_BLK;
        if (iblockIndex >= node->i_blk_cnt)
            goto failure;

        struct kfs_iblock *iblock = ((void *) kfs) + (node->i_blks[iblockIndex] * KFS_BLK_SZ);
        if (kfs_checksum(iblock, sizeof(struct kfs_iblock) - 4) != iblock->cksum) {
            kSerialPrintf("[KFS] iblock - Bad checksum\n");
            goto failure;
        }

        blockIndex = iblock->blks;
        blocknum = (blocknum - KFS_DIRECT_BLK) % KFS_INDIRECT_BLK;
        LOG("iblockIndex: %u (max %d) %d / blocknum = %u\n", iblockIndex, node->i_blk_cnt, iblock->blk_cnt, blocknum);
    } else {
        if (blocknum >= node->d_blk_cnt)
            goto failure;
        LOG("blocknum %u\n", blocknum);
    }

    struct kfs_block *block = ((void *) kfs) + (blockIndex[blocknum] * KFS_BLK_SZ);
    if (checkBlockChecksum(block) == 0) {
        kSerialPrintf("[KFS] block - Bad checksum\n");
        goto failure;
    }
    memcpy(buffer, block->data, block->usage);
    u32 ret = block->usage;

    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();
    return ret;

    failure:
    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();
    kSerialPrintf("[KFS] readblock failure\n");
    return -1;
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
