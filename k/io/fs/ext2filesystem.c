//
// Created by rebut_p on 26/12/18.
//

#include <string.h>
#include <kstdio.h>
#include <errno-base.h>
#include <fs.h>

#include <io/device/device.h>
#include <io/device/fscache.h>
#include <io/pit.h>
#include <system/kobject.h>
#include <system/allocator.h>

#include "ext2filesystem.h"
#include "filesystem.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static int ext2DeviceReadBlock(void *buf, u32 block, struct Ext2VolumeData *priv)
{
    u32 sectors_per_block = priv->sectors_per_block;
    if (!sectors_per_block)
        sectors_per_block = 1;

    LOG("[ext2] device read block %d in sectors [%d; %d] (sector per block: %d)\n",
        block, block * sectors_per_block, block * sectors_per_block + sectors_per_block, sectors_per_block);

    int tmp = fsCacheRead(priv->device, buf, sectors_per_block, block * sectors_per_block);
    LOG("read block value: %d\n", tmp);
    return tmp;
}

static void ext2DeviceWriteBlock(const void *buf, u32 block, struct Ext2VolumeData *priv)
{
    u32 sectors_per_block = priv->sectors_per_block;
    if (!sectors_per_block)
        sectors_per_block = 1;

    LOG("[ext2] device write block %d in sectors [%d; %d] (sector per block: %d)\n",
        block, block * sectors_per_block, block * sectors_per_block + sectors_per_block, sectors_per_block);

    fsCacheWrite(priv->device, buf, sectors_per_block, block * sectors_per_block);
}

static void ext2ReadInode(struct Ext2Inode *inode_buf, u32 inode, struct Ext2VolumeData *priv)
{
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

static void ext2WriteInode(struct Ext2Inode *inode_buf, u32 inode, struct Ext2VolumeData *priv)
{
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

    u32 final = bgd->block_of_inode_table + block;
    ext2DeviceReadBlock(block_buf, bgd->block_of_inode_table + block, priv);

    struct Ext2Inode *_inode = (struct Ext2Inode *) block_buf;
    _inode += (index % priv->inodes_per_block);

    memcpy(_inode, inode_buf, sizeof(struct Ext2Inode));
    ext2DeviceWriteBlock(block_buf, final, priv);
    kfree(block_buf);
}

static u32 ext2GetInodeBlock(u32 inode, u32 *b, u32 *ioff, struct Ext2VolumeData *priv)
{
    u32 bg = (inode - 1) / priv->sb.inodes_in_blockgroup;

    u32 nbBgPerBlock = priv->blocksize / sizeof(struct Ext2BlockGroupDesc);
    u32 bgblockId = bg / nbBgPerBlock;

    LOG("We seek BG %d (block: %d, offset: %d)\n", bg, bgblockId, bg % nbBgPerBlock);
    bg %= nbBgPerBlock;

    /* Now we have which BG the inode is in, load that desc */
    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return 0;

    ext2DeviceReadBlock(block_buf, priv->first_bgd + bgblockId, priv);
    struct Ext2BlockGroupDesc *bgd = (struct Ext2BlockGroupDesc *) block_buf;
    LOG("We seek BG %d\n", bg);
    /* Seek to the BG's desc */
    bgd += bg;

    /* Find the index and seek to the inode */
    u32 index = (inode - 1) % priv->sb.inodes_in_blockgroup;
    u32 block = (index * sizeof(struct Ext2Inode)) / priv->blocksize;
    LOG("index = %u, block = %u\n", index, block);

    index = index % priv->inodes_per_block;
    *b = block + bgd->block_of_inode_table;
    *ioff = index;

    LOG("index2 = %u, blockInodeTable = %u\n", index, bgd->block_of_inode_table);
    kfree(block_buf);
    return 1;
}

static int ext2ReadSinglyLinked(u32 singlyBlockId, u32 blockid, void *buf, struct Ext2VolumeData *priv)
{
    u32 *singlyBlock = kmalloc(priv->blocksize, 0, "singlyBlock");
    if (singlyBlock == NULL)
        return -1;

    ext2DeviceReadBlock(singlyBlock, singlyBlockId, priv);
    ext2DeviceReadBlock(buf, singlyBlock[blockid], priv);

    kfree(singlyBlock);
    return 0;
}

#define SIZE_OF_SINGLY (priv->blocksize * priv->blocksize / 4)

static int ext2ReadDoublyLinked(u32 doublyblockid, u32 blockid, void *buf, struct Ext2VolumeData *priv)
{

    u32 *doublyBlock = kmalloc(priv->blocksize, 0, "doublyBlock");
    if (doublyBlock == NULL)
        return -1;

    ext2DeviceReadBlock(doublyBlock, doublyblockid, priv);
    ext2ReadSinglyLinked(doublyBlock[blockid / SIZE_OF_SINGLY], blockid % SIZE_OF_SINGLY, buf, priv);

    kfree(doublyBlock);
    return 0;
}

#define SIZE_OF_DOUBLY (SIZE_OF_SINGLY * priv->blocksize)

static int ext2ReadTriplyLinked(u32 triplyblockid, u32 blockid, void *buf, struct Ext2VolumeData *priv)
{

    u32 *triplyBlock = kmalloc(priv->blocksize, 0, "triplyBlock");
    if (triplyBlock == NULL)
        return -1;

    ext2DeviceReadBlock(triplyBlock, triplyblockid, priv);
    ext2ReadDoublyLinked(triplyBlock[blockid / SIZE_OF_DOUBLY], blockid % SIZE_OF_DOUBLY, buf, priv);

    kfree(triplyBlock);
    return 0;
}

static int ext2ReadBlock(struct FsPath *path, char *buffer, u32 blocknum)
{
    LOG("[ext2] readblock: %u\n", blocknum);
    struct Ext2VolumeData *priv = path->volume->privateData;
    struct Ext2Inode pathInode;

    ext2ReadInode(&pathInode, path->inode, priv);
    if ((pathInode.type & 0xF000) == INODE_TYPE_DIRECTORY)
        return -1;

    if (blocknum < 12) {
        u32 b = pathInode.dbp[blocknum];
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
        ext2ReadSinglyLinked(pathInode.singly_block, blocknum - 12, buffer, priv);
    } else if (blocknum - 12 - (priv->blocksize / sizeof(u32)) < SIZE_OF_SINGLY) {
        u32 tmp = blocknum - 12 - (priv->blocksize / sizeof(u32));
        ext2ReadDoublyLinked(pathInode.doubly_block, tmp, buffer, priv);
    } else {
        u32 tmp = blocknum - 12 - (priv->blocksize / sizeof(u32)) - SIZE_OF_SINGLY;
        ext2ReadTriplyLinked(pathInode.triply_block, tmp, buffer, priv);
    }

    return (pathInode.size / priv->blocksize == blocknum ? pathInode.size % priv->blocksize : priv->blocksize);
}

static int ext2Stat(struct FsPath *path, struct stat *result)
{
    struct Ext2Inode pathInode;
    struct Ext2VolumeData *priv = path->volume->privateData;
    ext2ReadInode(&pathInode, path->inode, priv);

    result->st_ino = path->inode;
    result->st_uid = pathInode.uid;
    result->st_gid = pathInode.gid;
    result->st_size = pathInode.size;
    result->st_mode = pathInode.type;
    result->st_nlink = pathInode.hardlinks;
    result->st_blksize = path->volume->blockSize;

    result->st_atim = pathInode.last_access;
    result->st_mtim = pathInode.last_modif;
    result->st_ctim = pathInode.create_time;

    return 0;
}

static int ext2Readdir(struct FsPath *path, void *block, u32 nblock)
{
    if (nblock >= 12)
        return 0;

    struct Ext2VolumeData *priv = path->volume->privateData;
    struct Ext2Inode pathInode;

    ext2ReadInode(&pathInode, path->inode, priv);
    if ((pathInode.type & 0xF000) != INODE_TYPE_DIRECTORY)
        return -ENOTDIR;

    LOG("[ext2] readdir b=%d\n", nblock);
    u32 b = pathInode.dbp[nblock];
    if (b == 0)
        return -EFAULT;

    void *buf = kmalloc(priv->blocksize, 0, "newExt2Buf");
    if (buf == NULL)
        return -ENOMEM;

    u32 size = 0;
    ext2DeviceReadBlock(buf, b, priv);
    struct Ext2DirEntry *dir = buf;

    while ((u32) dir < (u32) buf + priv->blocksize && dir->inode != 0) {
        struct dirent tmpDirent;
        memcpy(tmpDirent.d_name, &dir->reserved + 1, dir->namelength);
        tmpDirent.d_name[dir->namelength] = '\0';
        tmpDirent.d_namlen = dir->namelength;
        tmpDirent.d_reclen = sizeof(struct dirent);
        tmpDirent.d_off = (size + 1) * sizeof(struct dirent);
        tmpDirent.d_ino = dir->inode;
        tmpDirent.d_type = FT_FILE;

        dir = (struct Ext2DirEntry *) ((u32) dir + dir->size);
        memcpy(block, &tmpDirent, sizeof(struct dirent));
        block += sizeof(struct dirent);
        size += 1;
    }

    kfree(buf);
    return size;
}

static u32 ext2_read_directory(const char *filename, void *buf, u32 blocksize)
{
    struct Ext2DirEntry *dir = buf;
    while ((u32) dir < (u32) buf + blocksize && dir->inode != 0) {
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

static u32 ext2FindNewInodeId(struct Ext2VolumeData *priv)
{
    /* Algorithm: Loop through the block group descriptors,
     * and find the number of unalloc inodes
     */
    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return 0;

    u32 id = 0;

    /* Loop through the block groups */
    ext2DeviceReadBlock(block_buf, priv->first_bgd, priv);
    struct Ext2BlockGroupDesc *bg = (struct Ext2BlockGroupDesc *) block_buf;
    for (u32 i = 0; i < priv->number_of_bgs; i++) {
        if (bg->num_of_unalloc_inode) {
            /* If the bg has some unallocated inodes,
             * find which inode is unallocated
             * This is easy:
             * For each bg we have sb->inodes_in_blockgroup inodes,
             * this one has num_of_unalloc_inode inodes unallocated,
             * therefore the latest id is:
             */
            id = ((i + 1) * priv->sb.inodes_in_blockgroup) - bg->num_of_unalloc_inode + 1;
            bg->num_of_unalloc_inode--;
            ext2DeviceWriteBlock(block_buf, priv->first_bgd + i, priv);

            /* Now, update the superblock as well */
            ext2DeviceReadBlock(block_buf, priv->sb.superblock_id, priv);
            struct Ext2Superblock *sb = (struct Ext2Superblock *) block_buf;
            sb->unallocatedinodes--;
            ext2DeviceWriteBlock(block_buf, priv->sb.superblock_id, priv);

            break;
        }
        bg++;
    }

    kfree(block_buf);
    return id;
}

static void ext2AllocBlock(u32 *out, struct Ext2VolumeData *priv)
{
    /* Algorithm: Loop through block group descriptors,
     * find which bg has a free block
     * and set that.
     */
    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return;

    ext2DeviceReadBlock(block_buf, priv->first_bgd, priv);
    struct Ext2BlockGroupDesc *bg = (struct Ext2BlockGroupDesc *) block_buf;
    for (u32 i = 0; i < priv->number_of_bgs; i++) {
        if (bg->num_of_unalloc_block) {
            *out = priv->sb.blocks - bg->num_of_unalloc_block + 1;
            bg->num_of_unalloc_block--;
            ext2DeviceWriteBlock(block_buf, priv->first_bgd + i, priv);

            LOG("Allocated block %d\n", *out);

            ext2DeviceReadBlock(block_buf, priv->sb.superblock_id, priv);
            struct Ext2Superblock *sb = (struct Ext2Superblock *) block_buf;
            sb->unallocatedblocks--;
            ext2DeviceWriteBlock(block_buf, priv->sb.superblock_id, priv);
            return;
        }
        bg++;
    }
}

static int ext2UnallocBlock(u32 blockId, struct Ext2VolumeData *priv)
{
    /* Algorithm: Loop through block group descriptors,
     * find which bg has a free block
     * and set that.

    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return;

    ext2DeviceReadBlock(block_buf, priv->first_bgd, priv);
    struct Ext2BlockGroupDesc *bg = (struct Ext2BlockGroupDesc *) block_buf;
    for (u32 i = 0; i < priv->number_of_bgs; i++) {
        if (bg->num_of_unalloc_block) {
            *out = priv->sb.blocks - bg->num_of_unalloc_block + 1;
            bg->num_of_unalloc_block--;
            ext2DeviceWriteBlock(block_buf, priv->first_bgd + i, priv);

            LOG("Allocated block %d\n", *out);

            ext2DeviceReadBlock(block_buf, priv->sb.superblock_id, priv);
            struct Ext2Superblock *sb = (struct Ext2Superblock *) block_buf;
            sb->unallocatedblocks--;
            ext2DeviceWriteBlock(block_buf, priv->sb.superblock_id, priv);
            return;
        }
        bg++;
    }
     */
    (void) blockId;
    (void) priv;
    return 0; // todo
}

static int ext2UpdateParentDir(struct Ext2DirEntry *entry, struct Ext2Inode *parentInode, struct FsPath *parentDir,
                               void *block_buf, struct Ext2VolumeData *priv)
{
    for (int i = 0; i < 12; i++) {
        if (parentInode->dbp[i] == 0 || parentInode->dbp[i] >= priv->sb.blocks) {
            u32 theblock = 0;
            ext2AllocBlock(&theblock, priv);
            parentInode->dbp[i] = theblock;
            parentInode->size += priv->blocksize;
            parentDir->size += priv->blocksize;
            ext2WriteInode(parentInode, parentDir->inode, priv);
            memset(block_buf, 0, priv->blocksize);
        } else {
            ext2DeviceReadBlock(block_buf, parentInode->dbp[i], priv);
        }
        struct Ext2DirEntry *d = (struct Ext2DirEntry *) block_buf;

        u32 passed = 0;
        while (d->inode != 0) {
            if (d->size == 0)
                break;

            /*u32 truesize = d->namelength + 8;
            truesize += 4 - truesize % 4;
            // u32 origsize = d->size;

            if (truesize != d->size) {
                d->size = (u16) truesize;
                passed += d->size;
                d = (struct Ext2DirEntry *) ((u32) d + d->size);
                entry->size = (u16)(priv->blocksize - passed);
                break;
            } else*/
            passed += d->size;
            d = (struct Ext2DirEntry *) ((u32) d + d->size);
        }

        if (passed >= priv->blocksize)
            continue;

        memcpy(d, entry, entry->size);
        ext2DeviceWriteBlock(block_buf, parentInode->dbp[i], priv);
        return 1;
    }
    return 0;
}

static struct FsPath *ext2InternalLink(u32 id, struct Ext2Inode *inode, struct FsPath *parentDir, const char *name)
{
    struct Ext2VolumeData *priv = parentDir->volume->privateData;
    struct Ext2Inode parentInode;
    ext2ReadInode(&parentInode, parentDir->inode, priv);

    void *block_buf = kmalloc(priv->blocksize, 0, "blockbufext2");
    if (block_buf == NULL)
        return NULL;

    u32 nameSize = strlen(name) + 1;
    struct Ext2DirEntry *entry = kmalloc(sizeof(struct Ext2DirEntry) + nameSize, 0, "ext2Dir");
    if (entry == NULL)
        goto failure_entry;

    entry->inode = id;
    entry->size = sizeof(struct Ext2DirEntry) + nameSize;
    entry->namelength = (u8) nameSize;
    entry->reserved = (u8) ((inode->type & 0xF000) == INODE_TYPE_DIRECTORY ? 2 : 1);
    memcpy(&entry->reserved + 1, name, nameSize);

    u32 block = 0; // The block where this inode should be written
    u32 ioff = 0;  // Offset into the block function to sizeof(inode_t)
    if (ext2GetInodeBlock(id, &block, &ioff, priv) == 0)
        goto failure_update;

    LOG("[ext2] link: block = %u, ioff = %u\n", block, ioff);

    ext2DeviceReadBlock(block_buf, block, priv);

    struct Ext2Inode *winode = (struct Ext2Inode *) block_buf + ioff;
    memcpy(winode, inode, sizeof(struct Ext2Inode));
    ext2DeviceWriteBlock(block_buf, block, priv);

    if (ext2UpdateParentDir(entry, &parentInode, parentDir, block_buf, priv) == 0)
        goto failure_update;

    struct FsPath *path = kmalloc(sizeof(struct FsPath), 0, "touchPathExt2");
    if (path == NULL)
        goto failure_update;

    kfree(entry);
    kfree(block_buf);

    path->inode = id;
    path->size = inode->size;
    path->mode = (u16) (inode->type & 0x0FFF);
    path->type = ((inode->type & 0xF000) == INODE_TYPE_DIRECTORY ? FS_FOLDER : FS_FILE);
    return path;

    failure_update:
    kfree(entry);
    failure_entry:
    kfree(block_buf);
    LOG("[ext2] link failure\n");
    return NULL;
}

static struct FsPath *ext2MkFile(struct FsPath *parentDir, const char *name, mode_t mode)
{
    u32 id;
    struct Ext2Inode inode;
    struct Ext2VolumeData *priv = parentDir->volume->privateData;
    LOG("[ext2] mkentry\n");

    memset(&inode, 0, sizeof(struct Ext2Inode));

    inode.hardlinks = 1;
    inode.size = 0;
    inode.type = (u16) (INODE_TYPE_FILE | mode);
    inode.disk_sectors = 2;
    inode.last_modif = inode.last_access = inode.create_time = (u32) gettick();

    id = ext2FindNewInodeId(priv);
    LOG("[ext2] mkentry: new id = %u\n", id);

    return ext2InternalLink(id, &inode, parentDir, name);
}

static struct FsPath *ext2MkDir(struct FsPath *parentDir, const char *name, mode_t mode)
{
    u32 id;
    struct Ext2Inode inode;
    struct Ext2VolumeData *priv = parentDir->volume->privateData;
    LOG("[ext2] mkdir (mode: %d)\n", mode);

    void *block_buf = kmalloc(priv->blocksize, 0, "blockbufext2");
    if (block_buf == NULL)
        return NULL;

    memset(&inode, 0, sizeof(struct Ext2Inode));

    inode.hardlinks = 1;
    inode.size = priv->blocksize;
    inode.type = (u16) (INODE_TYPE_DIRECTORY | mode);
    inode.disk_sectors = 2;
    inode.last_modif = inode.last_access = inode.create_time = (u32) gettick();

    id = ext2FindNewInodeId(priv);
    LOG("[ext2] mkentry: new id = %u\n", id);

    u32 theblock = 0;
    ext2AllocBlock(&theblock, priv);
    inode.dbp[0] = theblock;

    memset(block_buf, 0, priv->blocksize);
    struct Ext2DirEntry *entry1 = block_buf;
    entry1->inode = id;
    entry1->size = sizeof(struct Ext2DirEntry) + 2;
    entry1->namelength = 2;
    entry1->reserved = 2;
    memcpy(&entry1->reserved + 1, ".\0", 2);

    entry1 = (void *) (entry1) + entry1->size;
    entry1->inode = parentDir->inode;
    entry1->size = sizeof(struct Ext2DirEntry) + 3;
    entry1->namelength = 3;
    entry1->reserved = 2;
    memcpy(&entry1->reserved + 1, "..\0", 3);

    ext2DeviceWriteBlock(block_buf, inode.dbp[0], priv);

    kfree(block_buf);

    return ext2InternalLink(id, &inode, parentDir, name);
}

static struct FsPath *ext2Link(struct FsPath *nodeToLink, struct FsPath *parentDir, const char *filename)
{
    struct Ext2VolumeData *priv = parentDir->volume->privateData;
    struct Ext2Inode inode;
    u32 id;

    ext2ReadInode(&inode, nodeToLink->inode, priv);
    if ((inode.type & 0xF000) != INODE_TYPE_FILE)
        return NULL;

    id = nodeToLink->inode;
    inode.hardlinks += 1;

    return ext2InternalLink(id, &inode, parentDir, filename);
}

static int ext2ResizeFile(struct FsPath *path, u32 newSize)
{
    struct Ext2VolumeData *priv = path->volume->privateData;
    struct Ext2Inode pathInode;

    /* Locate and load the inode */
    ext2ReadInode(&pathInode, path->inode, priv);
    if ((pathInode.type & 0xF000) != INODE_TYPE_FILE)
        return -1;

    pathInode.size = newSize;
    LOG("[ext2] resize: %u (%u)\n", newSize, path->inode);
    ext2WriteInode(&pathInode, path->inode, priv);
    LOG("[ext2] resize end\n");
    path->size = newSize;
    return 0;
}

static int ext2WriteBlock(struct FsPath *path, const char *buffer, u32 blocknum)
{
    LOG("[ext2] writeblock: %u\n", blocknum);
    struct Ext2VolumeData *priv = path->volume->privateData;
    struct Ext2Inode pathInode;

    /* Locate and load the inode */
    ext2ReadInode(&pathInode, path->inode, priv);
    if ((pathInode.type & 0xF000) != INODE_TYPE_FILE)
        return -1;

    u32 bid = 0;
    if (blocknum < 12)
        bid = pathInode.dbp[blocknum];
    else {
        klog("[ext2] can not write more than 12kb\n"); // todo
        return -1;
    }

    if (bid == 0 || bid > priv->sb.blocks) {
        LOG("[ext2] writeblock: create new block\n");
        ext2AllocBlock(&bid, priv);
        pathInode.dbp[blocknum] = bid;
        LOG("[ext2] writeblock: id = %u\n", bid);
        ext2WriteInode(&pathInode, path->inode, priv);
    }

    ext2DeviceWriteBlock((void *) buffer, bid, priv);
    return priv->blocksize;
}

static struct FsPath *ext2Lookup(struct FsPath *path, const char *name)
{
    LOG("[ext2] Lookup: %s\n", name);
    struct Ext2VolumeData *priv = path->volume->privateData;
    struct Ext2Inode pathInode;

    ext2ReadInode(&pathInode, path->inode, priv);
    if ((pathInode.type & 0xF000) != INODE_TYPE_DIRECTORY)
        return NULL;

    void *buf = kmalloc(priv->blocksize, 0, "newExt2Buf");
    if (buf == NULL)
        return NULL;

    u32 fileInode = 0;

    for (int i = 0; i < 12; i++) {
        u32 b = pathInode.dbp[i];
        LOG("test loop %d: %u\n", i, b);
        if (b == 0) {
            LOG("[ext2] invalid dbp: %s\n", name);
            goto ext2LookupFaillure;
        }

        ext2DeviceReadBlock(buf, b, priv);
        fileInode = ext2_read_directory(name, buf, priv->blocksize);
        if (fileInode)
            break;
    }

    if (!fileInode)
        goto ext2LookupFaillure;

    LOG("Found inode %s! %d\n", name, fileInode);
    struct Ext2Inode inode;
    ext2ReadInode(&inode, fileInode, priv);

    struct FsPath *newPath = kmalloc(sizeof(struct FsPath), 0, "newExt2FsPath");
    if (newPath == NULL)
        goto ext2LookupFaillure;

    newPath->size = inode.size;
    newPath->mode = (u16) (inode.type & 0x0FFF);
    newPath->inode = fileInode;
    newPath->type = ((inode.type & 0xF000) == INODE_TYPE_DIRECTORY ? FS_FOLDER : FS_FILE);

    kfree(buf);
    return newPath;

    ext2LookupFaillure:
    kfree(buf);
    return NULL;
}

static int ext2Umount(struct FsVolume *volume)
{
    struct Ext2VolumeData *priv = volume->privateData;

    fsCacheFlush(priv->device);
    deviceDestroy(priv->device);

    kfree(volume->privateData);
    kfree(volume);
    return 0;
}

static int ext2Close(struct FsPath *path)
{
    kfree(path);
    return 0;
}

static struct FsPath *ext2Root(struct FsVolume *volume)
{
    LOG("[ext2] get root\n");
    struct Ext2VolumeData *priv = volume->privateData;
    struct Ext2Inode rootInode;

    ext2ReadInode(&rootInode, 2, priv);
    if ((rootInode.type & 0xF000) != INODE_TYPE_DIRECTORY) {
        klog("[ext2] FATAL: Root directory is not a directory! (%X)\n", (rootInode.type & 0xF000));
        goto ext2RootFaillure;
    }

    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "ext2Root");
    if (rootPath == NULL)
        goto ext2RootFaillure;

    rootPath->privateData = 0;
    rootPath->size = 0;
    rootPath->inode = 2;
    rootPath->type = FS_FOLDER;
    rootPath->mode = (u16) (rootInode.type & 0xFFF);
    return rootPath;

    ext2RootFaillure:
    klog("[ext2] get root dir faillure\n");
    return NULL;
}

struct FsVolume *ext2MountDevice(struct Device *device)
{
    LOG("[ext2] mount volume unit %u:\n", unit);
    struct Ext2VolumeData *priv = NULL;

    u8 *buf = (u8 *) kmalloc(1024, 0, "newBuf");
    if (buf == NULL)
        return NULL;

    deviceRead(device, buf, 2, 2);
    struct Ext2Superblock *sb = (struct Ext2Superblock *) buf;
    if (sb->ext2_sig != EXT2_SIGNATURE) {
        klog("Invalid EXT2 signature, have: 0x%x!\n", sb->ext2_sig);
        goto ext2MountFaillure;
    }

    klog("Valid EXT2 signature!: %d, %d, %d\n", sb->state, sb->os_id, sb->major_version);

    priv = (struct Ext2VolumeData *) kmalloc(sizeof(struct Ext2VolumeData), 0, "newstruct Ext2VolumeData");
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

    klog("There are %d block group(s).\n", number_of_bgs0);
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
    kfree(buf);
    kfree(priv);
    return NULL;
}

static struct FsVolume *ext2Mount(struct FsPath *dev)
{
    struct Kobject *device = fsOpenFile(dev, O_RDWR);
    if (device == NULL)
        return NULL;

    void *v = ext2MountDevice(device->data);
    koDestroy(device);
    return v;
}

static struct Kobject *ext2OpenFile(struct FsPath *path)
{
    path->refcount += 1;
    return koCreate((path->type == FS_FILE ? KO_FS_FILE : KO_FS_FOLDER), path, 0);
}

static int ext2Chmod(struct FsPath *path, mode_t mode)
{
    struct Ext2Inode pathInode;
    struct Ext2VolumeData *priv = path->volume->privateData;
    ext2ReadInode(&pathInode, path->inode, priv);

    pathInode.type = (u16) ((pathInode.type & 0xF000) | mode);

    ext2WriteInode(&pathInode, path->inode, priv);
    return 0;
}

static int ext2Unlink(struct FsPath *parent, struct FsPath *path)
{
    struct Ext2Inode pathInode;
    struct Ext2VolumeData *priv = path->volume->privateData;
    void *block_buf = kmalloc(priv->blocksize, 0, "buff");
    if (block_buf == NULL)
        return -ENOMEM;

    ext2ReadInode(&pathInode, path->inode, priv);

    if (pathInode.hardlinks <= 1) {
        for (int i = 0; i < 12; i++) {
            if (pathInode.dbp[i] == 0 || pathInode.dbp[i] >= priv->sb.blocks)
                continue;
            ext2UnallocBlock(pathInode.dbp[i], priv);
        }
        memset(&pathInode, 0, sizeof(struct Ext2Inode));
        ext2WriteInode(&pathInode, path->inode, priv);
        // todo free inode id
    } else {
        pathInode.hardlinks -= 1;
        ext2WriteInode(&pathInode, path->inode, priv);
    }

    struct Ext2Inode parentInode;
    ext2ReadInode(&parentInode, parent->inode, priv);

    for (int i = 0; i < 12; i++) {
        if (parentInode.dbp[i] == 0 || parentInode.dbp[i] >= priv->sb.blocks)
            continue;

        ext2DeviceReadBlock(block_buf, parentInode.dbp[i], priv);
        struct Ext2DirEntry *d = (struct Ext2DirEntry *) block_buf;

        struct Ext2DirEntry *entry = NULL;
        while ((void*) d < (block_buf + priv->blocksize)) {
            if (d->inode == path->inode) {
                entry = d;
                break;
            }
            d = (struct Ext2DirEntry *) ((u32) d + d->size);
        }

        if (entry == NULL)
            continue;

        entry->inode = 0;
        entry->reserved = 0;
        ext2DeviceWriteBlock(block_buf, parentInode.dbp[i], priv);
        break;
    }

    kfree(block_buf);
    return 0;
}

static struct Fs fs_ext2 = {
        "ext2fs",
        .mount = &ext2Mount,
        .umount = &ext2Umount,
        .root = &ext2Root,
        .openFile = &ext2OpenFile,
        .close = &ext2Close,
        .readdir = &ext2Readdir,
        .lookup = &ext2Lookup,
        .readBlock = &ext2ReadBlock,
        .writeBlock = &ext2WriteBlock,
        .stat = &ext2Stat,
        .resizeFile = &ext2ResizeFile,
        .mkfile = &ext2MkFile,
        .mkdir = &ext2MkDir,
        .link = &ext2Link,
        .unlink = &ext2Unlink,
        .chmod = &ext2Chmod
};

void initExt2FileSystem()
{
    fsRegister(&fs_ext2);
}