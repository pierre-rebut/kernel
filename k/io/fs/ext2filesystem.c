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

static void ext2DeviceWriteBlock(const void *buf, u32 block, struct Ext2PrivData *priv) {
    u32 sectors_per_block = priv->sectors_per_block;
    if (!sectors_per_block)
        sectors_per_block = 1;

    fsCacheWrite(priv->device, buf, sectors_per_block, block * sectors_per_block);
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

static void ext2WriteInode(struct Ext2Inode *inode_buf, u32 ii, struct Ext2PrivData *priv) {
    u32 bg = (ii - 1) / priv->sb.inodes_in_blockgroup;
    u32 i = 0;
    /* Now we have which BG the inode is in, load that desc */

    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return;

    ext2DeviceReadBlock(block_buf, priv->first_bgd, priv);

    struct Ext2BlockGroupDesc *bgd = (struct Ext2BlockGroupDesc *) block_buf;
    LOG("We seek BG %d\n", bg);
    /* Seek to the BG's desc */

    for (i = 0; i < bg; i++)
        bgd++;

    /* Find the index and seek to the inode */
    u32 index = (ii - 1) % priv->sb.inodes_in_blockgroup;
    LOG("Index of our inode is %d\n", index);
    u32 block = (index * sizeof(struct Ext2Inode)) / priv->blocksize;
    LOG("Relative: %d, Absolute: %d\n", block, bgd->block_of_inode_table + block);
    u32 final = bgd->block_of_inode_table + block;

    ext2DeviceReadBlock(block_buf, final, priv);
    struct Ext2Inode *_inode = (struct Ext2Inode *) block_buf;
    index = index % priv->inodes_per_block;
    for (i = 0; i < index; i++)
        _inode++;

    /* We have found the inode! */
    memcpy(_inode, inode_buf, sizeof(struct Ext2Inode));
    ext2DeviceReadBlock(block_buf, final, priv);
    kfree(block_buf);
}

static u32 ext2GetInodeBlock(u32 inode, u32 *b, u32 *ioff, struct Ext2PrivData *priv) {
    u32 bg = (inode - 1) / priv->sb.inodes_in_blockgroup;
    u32 i = 0;
    /* Now we have which BG the inode is in, load that desc */
    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return 0;

    ext2DeviceReadBlock(block_buf, priv->first_bgd, priv);
    struct Ext2BlockGroupDesc *bgd = (struct Ext2BlockGroupDesc *) block_buf;
    LOG("We seek BG %d\n", bg);
    /* Seek to the BG's desc */
    for (i = 0; i < bg; i++)
        bgd++;
    /* Find the index and seek to the inode */

    u32 index = (inode - 1) % priv->sb.inodes_in_blockgroup;
    u32 block = (index * sizeof(struct Ext2Inode)) / priv->blocksize;
    index = index % priv->inodes_per_block;
    *b = block + bgd->block_of_inode_table;
    *ioff = index;
    kfree(block_buf);
    return 1;
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
    struct Ext2Inode *pathInode = path->privateData;

    result->st_ino = path->inode;
    result->st_uid = pathInode->uid;
    result->st_gid = pathInode->gid;
    result->st_size = pathInode->size;
    result->st_mode = pathInode->type;
    result->st_nlink = pathInode->hardlinks;
    result->st_blksize = path->volume->blockSize;

    result->st_atim = pathInode->last_access;
    result->st_mtim = pathInode->last_modif;
    result->st_ctim = pathInode->create_time;

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

static u8
ext2FindFileInode(char *ff, struct Ext2Inode *root_buf, struct Ext2Inode *inode_buf, struct Ext2PrivData *priv) {
    char *filename = kmalloc(strlen(ff) + 1, 0, "filenameExt2");
    memcpy(filename, ff, strlen(ff) + 1);
    size_t n = strsplit(filename, '/');
    filename++; // skip the first crap
    u8 retnode = 0;
    if (n > 1) {
        /* Read inode#2 (Root dir) into inode */
        ext2DeviceReadBlock(inode_buf, 2, priv);
        /* Now, loop through the DPB's and see if it contains this filename */
        n--;
        while (n--) {
            LOG("Looking for: %s\n", filename);
            for (int i = 0; i < 12; i++) {
                u32 b = inode_buf->dbp[i];
                if (!b) break;
                ext2DeviceReadBlock(root_buf, b, priv);
                u32 rc = ext2_read_directory(filename, (struct Ext2DirEntry *) root_buf, priv->blocksize);
                if (!rc) {
                    if (strcmp(filename, "") == 0) {
                        kfree(filename);
                        return strcmp(ff, "/") ? retnode : 1;
                    }
                    LOG("File (%s (0x%x)) not found!\n", filename, filename);
                    kfree(filename);
                    return 0;
                } else {
                    /* inode now contains that inode
                     * get out of the for loop and continue traversing
                     */
                    retnode = rc;
                    goto fix;
                }
            }
            fix:;
            u32 s = strlen(filename);
            filename += s + 1;
        }
    } else {
        ext2DeviceReadBlock(inode_buf, 2, priv);
        if ((inode_buf->type & 0xF000) != INODE_TYPE_DIRECTORY) {
            kprintf("FATAL: Root directory is not a directory!\n");
            return 0;
        }
        /* We have found the directory!
         * Now, load the starting block
         */
        for (int i = 0; i < 12; i++) {
            u32 b = inode_buf->dbp[i];
            if (b == 0) break;
            ext2DeviceReadBlock(root_buf, b, priv);
            /* Now loop through the entries of the directory */
            if (ext2_read_directory(filename, (struct Ext2DirEntry *) root_buf, priv->blocksize))
                break;
        }
        /* This means the file is in the root directory */
    }
    kfree(filename);
    return retnode;
}

static void ext2FindNewInode_id(u32 *id, struct Ext2PrivData *priv) {
    /* Algorithm: Loop through the block group descriptors,
     * and find the number of unalloc inodes
     */
    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return;

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
            *id = ((i + 1) * priv->sb.inodes_in_blockgroup) - bg->num_of_unalloc_inode + 1;
            bg->num_of_unalloc_inode--;
            ext2DeviceWriteBlock(block_buf, priv->first_bgd + i, priv);
            /* Now, update the superblock as well */
            ext2DeviceReadBlock(block_buf, priv->sb.superblock_id, priv);
            struct Ext2Superblock *sb = (struct Ext2Superblock *) block_buf;
            sb->unallocatedinodes--;
            ext2DeviceWriteBlock(block_buf, priv->sb.superblock_id, priv);

            kfree(block_buf);
            return;
        }
        bg++;
    }

    kfree(block_buf);
}

static void ext2AllocBlock(u32 *out, struct Ext2PrivData *priv) {
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

u8 ext2Touch(const char *file, struct Ext2PrivData *priv) {
    if (!priv->device->driver->write)
        return 0;
    /* file = "/levex.txt"; */
    /* First create the inode */
    char *fil = kmalloc(strlen(file) + 1, 0, "filExt2");
    if (fil == NULL)
        return 0;

    memcpy(fil, file, strlen(file) + 1);

    struct Ext2Inode *fi = kmalloc(sizeof(struct Ext2Inode), 0, "inodeExt2");
    if (fi == NULL) {
        kfree(fil);
        return 0;
    }

    fi->hardlinks = 1;
    fi->size = 0;
    fi->type = INODE_TYPE_FILE;
    fi->disk_sectors = 2;
    /* Create the directory entry */
    size_t n = strsplit(fil, '/');
    n--;
    while (n) {
        fil += strlen(fil) + 1;
        n--;
    }
    size_t megaMallocLen = sizeof(struct Ext2DirEntry) + strlen(fil) + 1;
    megaMallocLen += sizeof(struct Ext2Inode) * 2 + priv->blocksize + strlen(file);
    void *megaMalloc = kmalloc(megaMallocLen, 0, "megamallocExt2");
    if (megaMalloc == NULL) {
        kfree(fi);
        kfree(fil);
        return 0;
    }

    //kprintf("filename: %s\n", fil);
    struct Ext2DirEntry *entry = megaMalloc;
    struct Ext2Inode *root_buf = megaMalloc + sizeof(struct Ext2DirEntry) + strlen(fil) + 1;
    struct Ext2Inode *inode = root_buf + 1;
    struct Ext2Inode *block_buf = root_buf + 2;
    char *f = (void *) (root_buf + 3);

    entry->size = sizeof(struct Ext2DirEntry) + strlen(fil) + 1;
    entry->namelength = strlen(fil) + 1;
    entry->reserved = 0;
    memcpy(&entry->reserved + 1, fil, strlen(fil) + 1);
    //kprintf("Length of dir entry: %d + %d = %d\n", sizeof(ext2_dir), strlen(fil), entry->size);
    /* Now, calculate this inode's id,
     * this is done from the superblock's inodes field
     * don't forget to update the superblock as well.
     */
    u32 id = 0;
    ext2FindNewInode_id(&id, priv);
    //kprintf("Inode id = %d\n", id);
    entry->inode = id;
    //ext2_read_directory(0, entry, dev, priv);
    /* Find where the previous inode is
     * and put this inode after this
     */
    u32 block = 0; /* The block where this inode should be written */
    u32 ioff = 0; /* Offset into the block function to sizeof(inode_t) */
    ext2GetInodeBlock(id, &block, &ioff, priv);
    //kprintf("This inode is located on block %d with ioff %d\n", block, ioff);
    /* First, read the block in */
    ext2DeviceReadBlock(root_buf, block, priv);
    struct Ext2Inode *winode = root_buf;
    for (u32 i = 0; i < ioff; i++)
        winode++;
    memcpy(winode, fi, sizeof(struct Ext2Inode));
    ext2DeviceWriteBlock(root_buf, block, priv);
    /* Now, we have added the inode, write the superblock as well. */
    //ext2_write_block(&priv->sb, priv->sb.superblock_id, dev, priv);
    /* Now, add the directory entry,
     * for this we have to locate the directory that holds us,
     * and find his inode.
     * We call ext2_find_file_inode() to place the inode to inode_buf
     */

    memcpy(f, file, strlen(file) + 1);
    str_backspace(f, '/');

    //kprintf("LF: %s\n", f);

    u32 t = ext2FindFileInode(f, root_buf, inode, priv);
    t++;
    //kprintf("Parent is inode %d\n", t);
    u8 found = 0;
    for (int i = 0; i < 12; i++) {
        /* Loop through the dpb to find an empty spot */
        if (inode->dbp[i] == 0) {
            /* This means we have not yet found a place for our entry,
             * and the inode has no block allocated.
             * Allocate a new block for this inode and place it there.
             */
            u32 theblock = 0;
            ext2AllocBlock(&theblock, priv);
            inode->dbp[i] = theblock;
            ext2WriteInode(inode, t, priv);
        }
        /* This DBP points to an array of directory entries */
        ext2DeviceReadBlock(block_buf, inode->dbp[i], priv);
        /* Loop throught the directory entries */
        struct Ext2DirEntry *d = (struct Ext2DirEntry *) block_buf;
        u32 passed = 0;
        while (d->inode != 0) {
            if (d->size == 0) break;
            u32 truesize = d->namelength + 8;
            //kprintf("true size has modulo 4 of %d, adding %d\n", truesize % 4, 4 - truesize%4);
            truesize += 4 - truesize % 4;
            u32 origsize = d->size;
            //kprintf("Truesize: %d Origsize: %d\n", truesize, origsize);
            if (truesize != d->size) {
                /* This is the last entry. Adjust the size to make space for our
                 * ext2_dir! Also, note that according to ext2-doc, entries must be on
                 * 4 byte boundaries!
                 */
                d->size = truesize;
                //kprintf("Adjusted entry len:%d, name %s!\n", d->size, &d->reserved + 1);
                /* Now, skip to the next */
                passed += d->size;
                d = (struct Ext2DirEntry *) ((u32) d + d->size);
                /* Adjust size */
                entry->size = priv->blocksize - passed;
                //kprintf("Entry size is now %d\n", entry->size);
                break;
            }
            //kprintf("skipped len: %d, name:%s!\n", d->size, &d->reserved + 1);
            passed += d->size;
            d = (struct Ext2DirEntry *) ((u32) d + d->size);
        }
        /* There is a problem, however. The last entry will always span the whole
         * block. We have to check if its size field is bigger than what it really is.
         * If it is, adjust its size, and add the entry after it, adjust our size
         * to span the block fully. If not, continue as we did before to the next DBP.
         */

        if (passed >= priv->blocksize) {
            //kprintf("Couldn't find it in DBP %d (%d > %d)\n", i, passed, priv->blocksize);
            continue;
        }
        /* Well, found a free entry! */
        //d = (ext2_dir *)((uint32_t)d + d->size);
        dir_write:
        memcpy(d, entry, entry->size);
        ext2DeviceWriteBlock(block_buf, inode->dbp[i], priv);
        //kprintf("Wrote to %d\n", inode->dbp[i]);
        return 1;
    }
    //kprintf("Couldn't write.\n");
    return 0;
}

static int ext2WriteBlock(struct FsPath *path, const char *buffer, u32 blocknum) {
    klog("[ext2] writeblock: %u\n", blocknum);
    struct Ext2PrivData *priv = path->volume->privateData;
    struct Ext2Inode *pathInode = path->privateData;

    /* Locate and load the inode */
    if (!(pathInode->type & INODE_TYPE_FILE))
        return -1;

    u32 bid = 0;
    if (blocknum < 12)
        bid = pathInode->dbp[blocknum];
    else {
        klog("[ext2] can not write more than 12kb\n");
        return -1;
    }

    if (bid == 0 || bid > priv->sb.blocks) {
        ext2AllocBlock(&bid, priv);
        pathInode->dbp[blocknum] = bid;
        pathInode->size += priv->blocksize;
        ext2WriteInode(pathInode, path->inode - 1, priv);
    }

    ext2DeviceWriteBlock((void *) buffer, bid, priv);
    return priv->blocksize;
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
    rootPath->inode = 2;
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
        .writeBlock = &ext2WriteBlock,
        .stat = &ext2Stat
};

void initExt2FileSystem() {
    fsRegister(&fs_ext2);
}