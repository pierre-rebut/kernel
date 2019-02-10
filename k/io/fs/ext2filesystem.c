//
// Created by rebut_p on 26/12/18.
//

#include <io/device/device.h>
#include <string.h>
#include <kstdio.h>
#include <sys/allocator.h>
#include <io/device/fscache.h>

#include "ext2filesystem.h"
#include "filesystem.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static int ext2DeviceReadBlock(void *buf, u32 block, struct Ext2PrivData *priv) {
    u32 sectors_per_block = priv->sectors_per_block;
    if (!sectors_per_block)
        sectors_per_block = 1;

    LOG("we want to read block %d which is sectors [%d; %d] (sector per block: %d)\n",
        block, block * sectors_per_block, block * sectors_per_block + sectors_per_block, sectors_per_block);

    int tmp = fsCacheRead(priv->device, buf, sectors_per_block, block * sectors_per_block);
    LOG("read block value: %d\n", tmp);
    return tmp;
}

static void ext2ReadInode(struct Ext2Inode *inode_buf, u32 inode, struct Ext2PrivData *priv) {
    u32 bg = (inode - 1) / priv->sb.inodes_in_blockgroup;

    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return;

    u32 nbBgPerBlock = priv->blocksize / sizeof(struct Ext2BlockGroupDesc);
    u32 bgblockId = bg / nbBgPerBlock;

    LOG("We seek BG %d (block: %d, offset: %d)\n", bg, bgblockId, bg % nbBgPerBlock);
    bg %= nbBgPerBlock;

    LOG("test %u, %u, %u\n", nbBgPerBlock, bgblockId, bg);

    ext2DeviceReadBlock(block_buf, priv->first_bgd + bgblockId, priv);

    struct Ext2BlockGroupDesc *bgd = (struct Ext2BlockGroupDesc *) block_buf;
    bgd += bg;

    LOG("%u, %u, %u, %u, %u, %u\n", bgd->block_of_block_usage_bitmap, bgd->block_of_inode_usage_bitmap,
        bgd->block_of_inode_table,
        bgd->num_of_unalloc_block, bgd->num_of_unalloc_inode, bgd->num_of_dirs
    );

    u32 index = (inode - 1) % priv->sb.inodes_in_blockgroup;
    LOG("Index of our inode is %d\n", index);
    u32 block = (index * sizeof(struct Ext2Inode)) / priv->blocksize;
    LOG("Relative: %d, Absolute: %d\n", block, bgd->block_of_inode_table + block);
    ext2DeviceReadBlock(block_buf, bgd->block_of_inode_table + block, priv);

    struct Ext2Inode *_inode = (struct Ext2Inode *) block_buf;
    _inode += (index % priv->inodes_per_block);

    memcpy(inode_buf, _inode, sizeof(struct Ext2Inode));
    kfree(block_buf);
}

static int ext2ReadSinglyLinked(u32 singlyBlockId, u32 blockid, void *buf, struct Ext2PrivData *priv) {
    u32 *singlyBlock = kmalloc(priv->blocksize, 0, "singlyBlock");
    if (singlyBlock == NULL)
        return -1;

    ext2DeviceReadBlock(singlyBlock, singlyBlockId, priv);
    ext2DeviceReadBlock(buf, singlyBlock[blockid], priv);

    kfree(singlyBlock);
    return 0;
}

#define SIZE_OF_SINGLY (priv->blocksize * priv->blocksize / 4)

static int ext2ReadDoublyLinked(u32 doublyblockid, u32 blockid, void *buf, struct Ext2PrivData *priv) {

    u32 *doublyBlock = kmalloc(priv->blocksize, 0, "doublyBlock");
    if (doublyBlock == NULL)
        return -1;

    ext2DeviceReadBlock(doublyBlock, doublyblockid, priv);
    ext2ReadSinglyLinked(doublyBlock[blockid / SIZE_OF_SINGLY], blockid % SIZE_OF_SINGLY, buf, priv);

    kfree(doublyBlock);
    return 0;
}

#define SIZE_OF_DOUBLY (SIZE_OF_SINGLY * priv->blocksize)

static int ext2ReadTriplyLinked(u32 triplyblockid, u32 blockid, void *buf, struct Ext2PrivData *priv) {

    u32 *triplyBlock = kmalloc(priv->blocksize, 0, "triplyBlock");
    if (triplyBlock == NULL)
        return -1;

    ext2DeviceReadBlock(triplyBlock, triplyblockid, priv);
    ext2ReadDoublyLinked(triplyBlock[blockid / SIZE_OF_DOUBLY], blockid % SIZE_OF_DOUBLY, buf, priv);

    kfree(triplyBlock);
    return 0;
}

static int ext2ReadBlock(struct FsPath *path, char *buffer, u32 blocknum) {
    LOG("[ext2] readblock: %u\n", blocknum);
    struct Ext2PrivData *priv = path->volume->privateData;
    struct Ext2Inode *pathInode = path->privateData;

    if ((pathInode->type & 0xF000) == INODE_TYPE_DIRECTORY)
        return -1;

    if (blocknum < 12) {
        u32 b = pathInode->dbp[blocknum];
        if (b == 0) {
            LOG("[ext2] readblock EOF\n");
            return 0;
        }

        if (b > priv->sb.blocks) {
            klog("[ext2] readblock: block %d outside range (max: %d)!\n", b, priv->sb.blocks);
            return -1;
        }

        LOG("Reading block: %d\n", b);
        ext2DeviceReadBlock((void *) buffer, b, priv);
    } else if (blocknum - 12 < (priv->blocksize / sizeof(u32))) {
        ext2ReadSinglyLinked(pathInode->singly_block, blocknum - 12, buffer, priv);
    } else if (blocknum - 12 - (priv->blocksize / sizeof(u32)) < SIZE_OF_SINGLY) {
        u32 tmp = blocknum - 12 - (priv->blocksize / sizeof(u32));
        ext2ReadDoublyLinked(pathInode->doubly_block, tmp, buffer, priv);
    } else {
        u32 tmp = blocknum - 12 - (priv->blocksize / sizeof(u32)) - SIZE_OF_SINGLY;
        ext2ReadTriplyLinked(pathInode->triply_block, tmp, buffer, priv);
    }

    return (pathInode->size / priv->blocksize == blocknum ? pathInode->size % priv->blocksize : priv->blocksize);
}

static int ext2Stat(struct FsPath *path, struct stat *result) {
    // struct Ext2PrivData *priv = path->volume->privateData;
    struct Ext2Inode *pathInode = path->privateData;

    result->inumber = 0;
    result->file_sz = pathInode->size;
    result->i_blk_cnt = 0;
    result->d_blk_cnt = 0;
    result->blk_cnt = 0;
    result->idx = 0;
    result->cksum = 0;
    return 0;
}

static struct dirent *ext2Readdir(struct FsPath *path, struct dirent *result) {
    struct Ext2PrivData *priv = path->volume->privateData;
    struct Ext2Inode *pathInode = path->privateData;

    if ((pathInode->type & 0xF000) != INODE_TYPE_DIRECTORY)
        return NULL;

    LOG("[ext2] readdir b=%d,tmp=%d\n", pathInode->tmpBreadir, pathInode->tmpDir);

    if (pathInode->tmpBreadir == 12)
        return NULL;

    u32 b = pathInode->dbp[pathInode->tmpBreadir];
    if (b == 0)
        return NULL;

    void *buf = kmalloc(priv->blocksize, 0, "newExt2Buf");
    if (buf == NULL)
        return NULL;

    ext2DeviceReadBlock(buf, b, priv);
    struct Ext2DirEntry *dir = (struct Ext2DirEntry *) (buf + pathInode->tmpDir);

    memcpy(result->d_name, &dir->reserved + 1, dir->namelength);
    result->d_name[dir->namelength] = 0;

    result->d_ino = dir->inode;
    result->d_type = FT_FILE;

    LOG("dir size = %u, %X\n", dir->size, dir);
    pathInode->tmpDir += dir->size;

    dir = (struct Ext2DirEntry *) ((u32) dir + dir->size);
    if ((u32) pathInode->tmpDir >= priv->blocksize || dir->inode == 0) {
        pathInode->tmpDir = 0;
        pathInode->tmpBreadir += 1;
    }

    kfree(buf);
    return result;
}

static u32 ext2_read_directory(const char *filename, void *buf, u32 blocksize) {
    struct Ext2DirEntry *dir = buf;
    while ((u32)dir < (u32)buf + blocksize && dir->inode != 0) {
        char *name = (char *) kmalloc(dir->namelength + 1, 0, "newName");
        memcpy(name, &dir->reserved + 1, dir->namelength);
        name[dir->namelength] = 0;
        LOG("DIR: %s (inode: %d, size: %d)\n", name, dir->inode, dir->size);

        if (strcmp(filename, name) == 0) {
            kfree(name);
            return dir->inode;
        }

        dir = (struct Ext2DirEntry *) ((u32) dir + dir->size);
        kfree(name);
    }
    return 0;
}

static struct FsPath *ext2Lookup(struct FsPath *path, const char *name) {
    // todo need fix
    LOG("[ext2] Lookup: %s\n", name);
    struct Ext2PrivData *priv = path->volume->privateData;
    struct Ext2Inode *pathInode = path->privateData;
    struct Ext2Inode *inode = NULL;

    if ((pathInode->type & 0xF000) != INODE_TYPE_DIRECTORY)
        return NULL;

    void *buf = kmalloc(priv->blocksize, 0, "newExt2Buf");
    if (buf == NULL)
        return NULL;

    u32 fileInode = 0;

    for (int i = 0; i < 12; i++) {
        u32 b = pathInode->dbp[i];
        LOG("test loop %d: %u\n", i, b);
        if (b == 0) {
            klog("[ext2] invalid dbp: %s\n", name);
            goto ext2LookupFaillure;
        }

        ext2DeviceReadBlock(buf, b, priv);
        fileInode = ext2_read_directory(name, buf, priv->blocksize);
        if (fileInode)
            break;
    }

    if (!fileInode)
        goto ext2LookupFaillure;

    inode = kmalloc(sizeof(struct Ext2Inode), 0, "newInode");
    if (inode == NULL)
        goto ext2LookupFaillure;

    LOG("Found inode %s! %d\n", name, fileInode);
    ext2ReadInode(inode, fileInode, priv);

    struct FsPath *newPath = kmalloc(sizeof(struct FsPath), 0, "newExt2FsPath");
    if (newPath == NULL)
        goto ext2LookupFaillure;

    inode->tmpBreadir = 0;
    inode->tmpDir = 0;

    newPath->privateData = inode;
    newPath->size = inode->size;
    newPath->inode = fileInode;

    kfree(buf);
    return newPath;

    ext2LookupFaillure:
    kfree(buf);
    kfree(inode);
    return NULL;
}

static int ext2Umount(struct FsVolume *volume) {
    struct Ext2PrivData *priv = volume->privateData;

    fsCacheFlush(priv->device);
    deviceDestroy(priv->device);

    kfree(volume->privateData);
    kfree(volume);
    return 0;
}

static int ext2Close(struct FsPath *path) {
    kfree(path->privateData);
    kfree(path);
    return 0;
}

static struct FsPath *ext2Root(struct FsVolume *volume) {
    LOG("[ext2] get root\n");
    struct Ext2PrivData *priv = volume->privateData;

    struct Ext2Inode *rootInode = kmalloc(sizeof(struct Ext2Inode), 0, "rootDataInode");
    if (rootInode == NULL)
        return NULL;

    ext2ReadInode(rootInode, 2, priv);
    if ((rootInode->type & 0xF000) != INODE_TYPE_DIRECTORY) {
        klog("FATAL: Root directory is not a directory! (%X)\n", (rootInode->type & 0xF000));
        goto ext2RootFaillure;
    }

    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "ext2Root");
    if (rootPath == NULL)
        goto ext2RootFaillure;

    rootPath->privateData = rootInode;
    rootPath->size = 0;
    return rootPath;

    ext2RootFaillure:
    klog("[ext2] get root dir faillure\n");
    kfree(rootInode);
    return NULL;
}

static struct FsVolume *ext2Mount(u32 unit) {
    LOG("[ext2] mount volume unit %u:\n", unit);
    struct Ext2PrivData *priv = NULL;

    u8 *buf = (u8 *) kmalloc(1024, 0, "newBuf");
    if (buf == NULL)
        return NULL;

    struct Device *device = deviceCreate("ata", unit);
    if (device == NULL)
        goto ext2MountFaillure;

    deviceRead(device, buf, 2, 2);
    struct Ext2Superblock *sb = (struct Ext2Superblock *) buf;
    if (sb->ext2_sig != EXT2_SIGNATURE) {
        klog("Invalid EXT2 signature, have: 0x%x!\n", sb->ext2_sig);
        goto ext2MountFaillure;
    }

    LOG("Valid EXT2 signature!: %d, %d, %d\n", sb->state, sb->os_id, sb->major_version);

    priv = (struct Ext2PrivData *) kmalloc(sizeof(struct Ext2PrivData), 0, "newstruct Ext2PrivData");
    if (priv == NULL)
        goto ext2MountFaillure;

    memcpy(&priv->sb, sb, sizeof(struct Ext2Superblock));

    LOG("test: %d, %d, inodesize: %d / %d, inode in blockgroup: %d\n",
        sb->inodes, sb->blocks, sb->inode_size, sizeof(struct Ext2Inode), sb->inodes_in_blockgroup);

    u32 blocksize = (u32) 1024 << sb->blocksize_hint;
    LOG("Size of a block: %d bytes\n", blocksize);
    priv->blocksize = blocksize;
    priv->inodes_per_block = blocksize / sizeof(struct Ext2Inode);
    priv->sectors_per_block = blocksize / 512;

    LOG("Size of volume: %d bytes\n", blocksize * (sb->blocks));
    u32 number_of_bgs0 = sb->blocks / sb->blocks_in_blockgroup;
    if (!number_of_bgs0)
        number_of_bgs0 = 1;

    LOG("There are %d block group(s).\n", number_of_bgs0);
    priv->number_of_bgs = number_of_bgs0;

    u32 block_bgdt = sb->superblock_id + (sizeof(struct Ext2Superblock) / blocksize);
    priv->first_bgd = block_bgdt;
    LOG("first_bgd: %d\n", block_bgdt);

    priv->device = device;

    struct FsVolume *ext2Volume = kmalloc(sizeof(struct FsVolume), 0, "newExt2Volume");
    if (ext2Volume == NULL)
        goto ext2MountFaillure;

    ext2Volume->blockSize = blocksize;
    ext2Volume->privateData = priv;
    kfree(buf);
    return ext2Volume;

    ext2MountFaillure:
    klog("[ext2] failure\n");
    deviceDestroy(device);
    kfree(buf);
    kfree(priv);
    return NULL;
}

static struct Fs fs_ext2 = {
        "ext2fs",
        .mount = &ext2Mount,
        .umount = &ext2Umount,
        .root = &ext2Root,
        .close = &ext2Close,
        .readdir = &ext2Readdir,
        .lookup = &ext2Lookup,
        .readBlock = &ext2ReadBlock,
        .stat = &ext2Stat
};

void initExt2FileSystem() {
    fsRegister(&fs_ext2);
}