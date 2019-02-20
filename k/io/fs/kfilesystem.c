//
// Created by rebut_p on 23/09/18.
//

#include "kfilesystem.h"
#include "filesystem.h"

#include <k/kfs.h>
#include <kstdio.h>
#include <string.h>
#include <cpu.h>
#include <sys/paging.h>
#include <sys/allocator.h>
#include <sys/physical-memory.h>
#include <errno-base.h>

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define KFS_MEM_POS 0x1400000

static char checkBlockChecksum(struct kfs_block *block)
{
    u32 cksum = block->cksum;
    block->cksum = 0;
    u32 tmp = kfs_checksum(block, sizeof(struct kfs_block));
    block->cksum = cksum;

    return tmp == cksum;
}

static struct kfs_inode *getFileINode(const char *pathname)
{
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

static struct FsPath *kfsRoot(struct FsVolume *volume)
{
    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "kfsRoot");
    if (!rootPath)
        return NULL;

    struct kfs_superblock *kfs = (struct kfs_superblock *) KFS_MEM_POS;

    cli();
    pagingSwitchPageDirectory(volume->privateData);
    rootPath->privateData = ((void *) kfs + (kfs->inode_idx * KFS_BLK_SZ));
    rootPath->inode = 0;
    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();

    rootPath->type = FS_FOLDER;
    return rootPath;
}

static struct FsVolume *kfsMount(struct FsPath *file)
{
    LOG("KFS mount: %s - %u - %p\n", (const char *) data, currentTask->pid, currentTask->pageDirectory);

    void *pd = NULL;
    void *tmpData = NULL;

    LOG("[KFS] alloc tmp data\n");
    tmpData = kmalloc(4096, 0, "kfsTmp");
    if (tmpData == NULL)
        goto kfsMountFailure;

    LOG("[KFS] create page directory\n");
    pd = pagingCreatePageDirectory();
    if (!pd)
        goto kfsMountFailure;

    u32 lengthFile = 0;
    u32 allocSize = alignUp(file->size, PAGESIZE);
    LOG("[KFS] alloc page %p (addr: %X, size: %u)\n", pd, KFS_MEM_POS, allocSize);

    LOG("[KFS] mount2: %u - %p\n", currentTask->pid, currentTask->pageDirectory);
    if (pagingAlloc(pd, (void *) KFS_MEM_POS, allocSize, MEM_WRITE))
        goto kfsMountFailure;

    LOG("[KFS] mount3: %u - %p\n", currentTask->pid, currentTask->pageDirectory);

    LOG("[KFS] read file and put into page alloc\n");
    while (lengthFile < file->size) {
        LOG("[KFS] read data from file\n");
        int tmp = fsReadFile(file, tmpData, 4096, lengthFile);
        LOG("[KFS] result: %d\n", tmp);
        if (tmp < 0)
            goto kfsMountFailure;
        if (tmp == 0)
            break;

        LOG("[KFS] memcpy to page alloc\n");
        cli();
        pagingSwitchPageDirectory(pd);
        memcpy((void *) KFS_MEM_POS + lengthFile, tmpData, (u32) tmp);
        pagingSwitchPageDirectory(currentTask->pageDirectory);
        sti();
        lengthFile += tmp;
    }

    LOG("[KFS] check kfs superblock\n");
    struct kfs_superblock *tmp = (struct kfs_superblock *) KFS_MEM_POS;

    cli();
    pagingSwitchPageDirectory(pd);
    int res = (tmp->magic != KFS_MAGIC || kfs_checksum(tmp, sizeof(struct kfs_superblock) - 4) != tmp->cksum);
    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();

    if (res) {
        klog("KFS - Bad superblock\n");
        goto kfsMountFailure;
    }

    LOG("[KFS] create FsVolume\n");
    struct FsVolume *kfsVolume = kmalloc(sizeof(struct FsVolume), 0, "newKfsVolume");
    if (kfsVolume == NULL)
        goto kfsMountFailure;

    kfsVolume->blockSize = KFS_BLK_DATA_SZ;
    kfsVolume->privateData = pd;
    kfree(tmpData);
    return kfsVolume;

    kfsMountFailure:
    kfree(tmpData);
    pagingDestroyPageDirectory(pd);
    return NULL;
}

static int kfsUmount(struct FsVolume *volume)
{
    pagingDestroyPageDirectory(volume->privateData);
    kfree(volume);
    return 0;
}

static int kfsClose(struct FsPath *path)
{
    kfree(path);
    return 0;
}

static int kfsStat(struct FsPath *path, struct stat *result)
{
    struct kfs_inode *inode = path->privateData;

    cli();
    pagingSwitchPageDirectory(path->volume->privateData);

    result->st_ino = inode->inumber;
    result->st_size = inode->file_sz;
    result->st_blksize = KFS_BLK_DATA_SZ;
    result->st_mode = 0;

    result->st_uid = result->st_gid = result->st_nlink = 0;
    result->st_atim = result->st_ctim = result->st_mtim = 0;

    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();
    return 0;
}

static struct FsPath *kfsLookup(struct FsPath *path, const char *name)
{
    LOG("[KFS] lookup fct\n");
    struct kfs_inode *node;
    u32 filesize = 0;
    u32 inode = 0;
    enum FS_TYPE type = FS_FILE;

    if (*name == 0 || strcmp(name, ".") == 0) {
        node = path->privateData;
        type = FS_FOLDER;
    } else {
        cli();
        pagingSwitchPageDirectory(path->volume->privateData);
        node = getFileINode(name);
        if (node != NULL) {
            filesize = node->file_sz;
            inode = node->idx;
        }
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
    file->mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    file->size = filesize;
    file->inode = inode;
    file->type = type;
    return file;
}

static int kfsReaddir(struct FsPath *path, void *block, u32 nblock)
{
    struct kfs_superblock *kfs = (struct kfs_superblock *) KFS_MEM_POS;
    LOG("[KFS] readdir: %u\n", nblock);

    void *tmpBlock = kmalloc(DIRENT_BUFFER_SIZE, 0, "tmpKfsReadir");
    if (tmpBlock == NULL)
        return -ENOMEM;

    cli();
    pagingSwitchPageDirectory(path->volume->privateData);

    if (nblock > kfs->inode_cnt / DIRENT_BUFFER_NB) {
        pagingSwitchPageDirectory(currentTask->pageDirectory);
        sti();
        return 0;
    }

    struct kfs_inode *dir = (struct kfs_inode *) (KFS_MEM_POS + (kfs->inode_idx * KFS_BLK_SZ));
    for (u32 i = 0; i < nblock; i++) {
        dir = (struct kfs_inode *) (KFS_MEM_POS + (dir->next_inode * KFS_BLK_SZ));
    }

    u32 size = 0;
    for (u32 i = 0; i < DIRENT_BUFFER_NB; i++) {
        if (dir->next_inode == 0)
            break;

        struct dirent tmpDirent;
        strcpy(tmpDirent.d_name, dir->filename);
        tmpDirent.d_ino = dir->idx;
        tmpDirent.d_type = FT_FILE;
        tmpDirent.d_namlen = strlen(dir->filename);

        klog("bite en bois de chaine: %d\n", i);
        memcpy(tmpBlock, &tmpDirent, sizeof(struct dirent));
        klog("bite en marbre\n");
        block += sizeof(struct dirent);
        size += 1;

        dir = (struct kfs_inode *) (KFS_MEM_POS + (dir->next_inode * KFS_BLK_SZ));
    }

    pagingSwitchPageDirectory(currentTask->pageDirectory);
    sti();

    memcpy(block, tmpBlock, size * sizeof(struct dirent));
    kfree(tmpBlock);
    return size;
}

static int kfsReadBlock(struct FsPath *path, char *buffer, u32 blocknum)
{
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
            klog("[KFS] iblock - Bad checksum\n");
            goto failure;
        }

        blockIndex = iblock->blks;
        blocknum = (blocknum - KFS_DIRECT_BLK) % KFS_INDIRECT_BLK;
        LOG("[KFS] iblockIndex: %u (max %d) %d / blocknum = %u\n", iblockIndex, node->i_blk_cnt, iblock->blk_cnt,
            blocknum);
    } else {
        if (blocknum >= node->d_blk_cnt)
            goto failure;
        LOG("[KFS] blocknum %u\n", blocknum);
    }

    struct kfs_block *block = ((void *) kfs) + (blockIndex[blocknum] * KFS_BLK_SZ);
    if (checkBlockChecksum(block) == 0) {
        klog("[KFS] block - Bad checksum\n");
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
    klog("[KFS] readblock failure\n");
    return -1;
}

static struct Kobject *kfsOpenFile(struct FsPath *path)
{
    path->refcount += 1;
    return koCreate((path->type == FS_FILE ? KO_FS_FILE : KO_FS_FOLDER), path, 0);
}

static struct Fs fs_kfs = {
        .name = "kfs",
        .mount = &kfsMount,
        .umount = &kfsUmount,
        .root = &kfsRoot,
        .openFile = &kfsOpenFile,
        .lookup = &kfsLookup,
        .readdir = &kfsReaddir,
        .readBlock = &kfsReadBlock,
        .stat = &kfsStat,
        .close = &kfsClose
};

void initKFileSystem()
{
    fsRegister(&fs_kfs);
}
