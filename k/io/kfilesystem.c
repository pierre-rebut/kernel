//
// Created by rebut_p on 23/09/18.
//

#include "kfilesystem.h"
#include "filesystem2.h"

#include <stdio.h>
#include <sys/allocator.h>
#include <string.h>

static struct kfs_superblock *kfs = NULL;
static struct Fs fs_kfs = {
        .name = "kfs"
};

void initKFileSystem(module_t *module) {
    struct kfs_superblock *tmp = (struct kfs_superblock *) module->mod_start;

    if (tmp->magic != KFS_MAGIC) {
        kSerialPrintf("KFS - Bad magic number\n");
        return;
    }

    if (kfs_checksum(tmp, sizeof(struct kfs_superblock) - 4) != tmp->cksum) {
        kSerialPrintf("KFS - Bad checksum\n");
        return;
    }

    kfs = tmp;
    fs_register(&fs_kfs);
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
        kSerialPrintf("KFS read - Bad checksum\n");
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

void *kfsOpen(const char *pathname, int flags) {
    if (kfs == NULL || pathname == NULL || flags != O_RDONLY)
        return NULL;

    if (pathname[0] == '/')
        pathname++;

    struct kfs_inode *node = getFileINode(pathname);
    if (node == NULL)
        return NULL;

    struct file_entry *file = kmalloc(sizeof(struct file_entry), 0, "file entry");
    if (!file)
        return NULL;

    file->blockIndex = 0;
    file->d_blks = node->d_blks;

    if (changeBlock(file) == -1) {
        kfree(file);
        return NULL;
    }

    file->node = node;
    file->iblock = NULL;
    file->iblockIndex = 0;
    file->flags = flags;
    file->dataIndex = 0;

    struct FileDescriptor *fd = kmalloc(sizeof(struct FileDescriptor), 0, "file descriptor 2");
    if (!fd) {
        kfree(file);
        return NULL;
    }

    fd->privateData = file;
    fd->writeFct = NULL;
    fd->readFct = &kfsRead;
    fd->seekFct = &kfsSeek;
    fd->closeFct = &kfsClose;
    fd->statFct = &kfsFStat;
    return fd;
}

static char changeIBlock(struct file_entry *file, struct kfs_inode *node) {
    if (file->iblockIndex >= node->i_blk_cnt) {
        kSerialPrintf("KFS end of file\n");
        return -1;
    }

    file->iblock = ((void *) kfs) + (node->i_blks[file->iblockIndex] * KFS_BLK_SZ);
    if (kfs_checksum(file->iblock, sizeof(struct kfs_iblock) - 4) != file->iblock->cksum) {
        kSerialPrintf("KFS iblock - Bad checksum\n");
        return -1;
    }

    file->d_blks = file->iblock->blks;
    file->blockIndex = 0;
    file->iblockIndex++;

    return 0;
}

s32 kfsRead(void *privateData, void *buf, u32 size) {
    if (privateData == NULL || kfs == NULL)
        return -1;

    struct file_entry *file = (struct file_entry *) privateData;
    struct kfs_inode *node = file->node;
    u8 *buffer = (u8 *) buf;

    for (u32 i = 0; i < size; i++) {
        if (file->dataIndex >= file->block->usage) {
            if ((file->iblock == NULL && file->blockIndex >= node->d_blk_cnt) ||
                (file->iblock != NULL && file->blockIndex >= file->iblock->blk_cnt)) {
                char res = changeIBlock(file, node);
                if (res == -1)
                    return i > 0 ? (s32) i : -1;
            }
            if (changeBlock(file) == -1)
                return i > 0 ? (s32) i : -1;
            file->dataIndex = 0;
        }

        buffer[i] = file->block->data[file->dataIndex];
        file->dataIndex++;
    }

    return (s32) size;
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

off_t kfsSeek(void *privateData, off_t offset, int whence) {
    if (!privateData || !kfs)
        return -1;

    struct file_entry *file = (struct file_entry *) privateData;
    if (offset >= (off_t) file->node->file_sz)
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
    return kfsSeek(privateData, offset, offset < 0 ? SEEK_END : SEEK_SET);
}

int kfsClose(void *privateData) {
    if (!privateData)
        return -1;

    kfree(privateData);
    return 0;
}

void kfsListFiles() {
    if (kfs == NULL)
        return;

    kSerialPrintf("KFS - Files: (%d)\n", kfs->inode_cnt);

    struct kfs_inode *node = (struct kfs_inode *) ((void *) kfs + (kfs->inode_idx * KFS_BLK_SZ));
    for (u32 i = 0; i < kfs->inode_cnt; i++) {
        if (kfs_checksum(node, sizeof(struct kfs_inode) - 4) != node->cksum) {
            kSerialPrintf("KFS node - Bad checksum\n");
            kfs = NULL;
            return;
        }

        kSerialPrintf("%d - file: %s, size: %d (cnt: %d, db_cnt: %d, ib_cnt: %d)\n", node->inumber, node->filename,
                      node->file_sz, node->blk_cnt, node->d_blk_cnt, node->i_blk_cnt);
        node = (struct kfs_inode *) ((void *) kfs + (node->next_inode * KFS_BLK_SZ));
    }
}

u32 kfsLengthOfFile(const char *pathname) {
    if (kfs == NULL || pathname == NULL)
        return 0;

    if (pathname[0] == '/')
        pathname++;

    struct kfs_inode *node = getFileINode(pathname);
    if (node == NULL)
        return 0;

    return node->file_sz;
}

static void getStatInfo(struct kfs_inode *inode, struct stat *data) {
    data->inumber = inode->inumber;
    data->file_sz = inode->file_sz;
    data->i_blk_cnt = inode->i_blk_cnt;
    data->d_blk_cnt = inode->d_blk_cnt;
    data->blk_cnt = inode->blk_cnt;
    data->idx = inode->idx;
    data->cksum = inode->cksum;
}

int kfsStat(const char *pathname, struct stat *data) {
    struct kfs_inode *node = getFileINode(pathname);
    if (node == NULL)
        return -1;
    getStatInfo(node, data);
    return 0;
}

int kfsFStat(void *privateData, struct stat *data) {
    if (!privateData || !kfs)
        return -1;

    struct file_entry *file = (struct file_entry *) privateData;
    getStatInfo(file->node, data);

    return 0;
}

struct FolderDescriptor *kfsOpendir(const char *pathname) {
    if (kfs == NULL)
        return NULL;

    (void)pathname;

    struct kfs_inode *node = kmalloc(sizeof(struct kfs_inode), 0, "folder inode");
    if (!node)
        return NULL;

    memcpy(node, ((void *) kfs + (kfs->inode_idx * KFS_BLK_SZ)), sizeof(struct kfs_inode));

    struct FolderDescriptor *fd = kmalloc(sizeof(struct FolderDescriptor), 0, "folderDescriptor");
    if (!fd) {
        kfree(node);
        return NULL;
    }

    fd->privateData = node;
    fd->readFct = &kfsReaddir;
    fd->closeFct = &kfsClose;
    return fd;
}

struct dirent *kfsReaddir(void *entry, struct dirent *data) {
    if (entry == NULL || kfs == NULL)
        return NULL;

    struct kfs_inode *node = (struct kfs_inode*)entry;
    if (node->idx > kfs->inode_cnt)
        return NULL;

    strcpy(data->d_name, node->filename);
    data->d_ino = node->idx;

    node = (struct kfs_inode *) ((void *) kfs + (node->next_inode * KFS_BLK_SZ));
    memcpy(entry, node, sizeof(struct kfs_inode));

    return data;
}