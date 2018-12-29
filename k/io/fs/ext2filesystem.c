//
// Created by rebut_p on 26/12/18.
//

#include <io/device/device.h>
#include <string.h>
#include <stdio.h>
#include <sys/allocator.h>
#include <io/device/fscache.h>

#include "ext2filesystem.h"
#include "filesystem.h"

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
#define LOG(x, ...)

static int ext2_read_block(void *buf, u32 block, struct Ext2PrivData *priv) {
    u32 sectors_per_block = priv->sectors_per_block;
    if (!sectors_per_block)
        sectors_per_block = 1;

    LOG("we want to read block %d which is sectors [%d; %d] (sector per block: %d)\n",
        block, block * sectors_per_block, block * sectors_per_block + sectors_per_block, sectors_per_block);

    int tmp = fsCacheRead(priv->device, buf, sectors_per_block, block * sectors_per_block);
    LOG("read block value: %d\n", tmp);
    return tmp;
}

static int ext2_write_block(void *buf, u32 block, struct Ext2PrivData *priv) {
    u32 sectors_per_block = priv->sectors_per_block;
    if (!sectors_per_block)
        sectors_per_block = 1;

    return deviceWrite(priv->device, buf, block * sectors_per_block, sectors_per_block);
}

static void ext2_read_inode(struct Ext2Inode *inode_buf, u32 inode, struct Ext2PrivData *priv) {
    u32 bg = (inode - 1) / priv->sb.inodes_in_blockgroup;

    void *block_buf = kmalloc(priv->blocksize, 0, "blockBuf");
    if (block_buf == NULL)
        return;

    u32 nbBgPerBlock = priv->blocksize / sizeof(struct Ext2BlockGroupDesc);
    u32 bgblockId = bg / nbBgPerBlock;

    LOG("We seek BG %d (block: %d, offset: %d)\n", bg, bgblockId, bg % nbBgPerBlock);
    bg %= nbBgPerBlock;

    LOG("test %u, %u, %u\n", nbBgPerBlock, bgblockId, bg);

    ext2_read_block(block_buf, priv->first_bgd + bgblockId, priv);

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
    ext2_read_block(block_buf, bgd->block_of_inode_table + block, priv);

    struct Ext2Inode *_inode = (struct Ext2Inode *) block_buf;
    _inode += (index % priv->inodes_per_block);

    memcpy(inode_buf, _inode, sizeof(struct Ext2Inode));
    kfree(block_buf);
}
/*
void ext2_write_inode(struct Ext2Inode *inode_buf, u32 ii, struct Device *dev, ext2_priv_data *priv) {
    u32 bg = (ii - 1) / priv->sb.inodes_in_blockgroup;
    u32 i = 0;
    // Now we have which BG the inode is in, load that desc
    if (!block_buf) block_buf = (u8 *) kmalloc(priv->blocksize, 0, "newBlockBuf");
    ext2_read_block(block_buf, priv->first_bgd, dev, priv);
    block_group_desc_t *bgd = (block_group_desc_t *) block_buf;
    LOG("We seek BG %d\n", bg);
    // Seek to the BG's desc
    for (i = 0; i < bg; i++)
        bgd++;
    // Find the index and seek to the inode
    u32 index = (ii - 1) % priv->sb.inodes_in_blockgroup;
    LOG("Index of our inode is %d\n", index);
    u32 block = (index * sizeof(struct Ext2Inode)) / priv->blocksize;
    LOG("Relative: %d, Absolute: %d\n", block, bgd->block_of_inode_table + block);
    u32 final = bgd->block_of_inode_table + block;
    ext2_read_block(block_buf, final, dev, priv);
    struct Ext2Inode *_inode = (struct Ext2Inode *) block_buf;
    index = index % priv->inodes_per_block;
    for (i = 0; i < index; i++)
        _inode++;
    // We have found the inode!
    memcpy(_inode, inode_buf, sizeof(struct Ext2Inode));
    ext2_write_block(block_buf, final, dev, priv);
}
 */
/*
u32 ext2_get_inode_block(u32 inode, u32 *b, u32 *ioff, struct Device *dev, ext2_priv_data *priv) {
    u32 bg = (inode - 1) / priv->sb.inodes_in_blockgroup;
    u32 i = 0;
    // Now we have which BG the inode is in, load that desc
    if (!block_buf) block_buf = (u8 *) kmalloc(priv->blocksize, 0, "newBlockBuf");
    ext2_read_block(block_buf, priv->first_bgd, dev, priv);
    block_group_desc_t *bgd = (block_group_desc_t *) block_buf;
    LOG("We seek BG %d\n", bg);
    // Seek to the BG's desc
    for (i = 0; i < bg; i++)
        bgd++;
    // Find the index and seek to the inode
    u32 index = (inode - 1) % priv->sb.inodes_in_blockgroup;
    u32 block = (index * sizeof(struct Ext2Inode)) / priv->blocksize;
    index = index % priv->inodes_per_block;
    *b = block + bgd->block_of_inode_table;
    *ioff = index;
    return 1;
}*/

/*
u8 ext2_find_file_inode(char *ff, struct Ext2Inode *inode_buf, struct Device *dev, ext2_priv_data *priv) {
    char *filename = kmalloc(strlen(ff) + 1, 0, "newFilename");
    memcpy(filename, ff, strlen(ff) + 1);
    size_t n = strsplit(filename, '/');
    filename++; // skip the first crap
    u32 retnode = 0;
    if (n > 1) {
        // Read inode#2 (Root dir) into inode
        ext2_read_inode(inode, 2, dev, priv);
        // Now, loop through the DPB's and see if it contains this filename
        n--;
        while (n--) {
            LOG("Looking for: %s\n", filename);
            for (int i = 0; i < 12; i++) {
                u32 b = inode->dbp[i];
                if (!b) break;
                ext2_read_block(root_buf, b, dev, priv);
                u32 rc = ext2_read_directory(filename, (ext2_dir *) root_buf, dev, priv);
                if (!rc) {
                    if (strcmp(filename, "") == 0) {
                        kfree(filename);
                        return strcmp(ff, "/") ? retnode : 1;
                    }
                    LOG("File (%s (0x%x)) not found!\n", filename, filename);
                    kfree(filename);
                    return 0;
                } else {
                    // inode now contains that inode
                    // get out of the for loop and continue traversing

                    retnode = rc;
                    goto fix;
                }
            }
            fix:;
            u32 s = strlen(filename);
            filename += s + 1;
        }
        memcpy(inode_buf, inode, sizeof(struct Ext2Inode));
    } else {
        // This means the file is in the root directory
        ext2_read_root_directory(filename, dev, priv);
        memcpy(inode_buf, inode, sizeof(struct Ext2Inode));
    }
    kfree(filename);
    return retnode;
}*/
/*
void ext2_list_directory(char *dd, char *buffer, struct Device *dev, ext2_priv_data *priv) {
    char *dir = dd;
    int rc = ext2_find_file_inode(dir, (struct Ext2Inode *) buffer, dev, priv);
    if (!rc) return;
    for (int i = 0; i < 12; i++) {
        u32 b = inode->dbp[i];
        if (!b) break;
        ext2_read_block(root_buf, b, dev, priv);
        ext2_read_directory(0, (ext2_dir *) root_buf, dev, priv);
    }
}
*/
/*
void ext2_find_new_inode_id(u32 *id, struct Device *dev, ext2_priv_data *priv) {
    // Algorithm: Loop through the block group descriptors,
    // and find the number of unalloc inodes

    // Loop through the block groups
    ext2_read_block(root_buf, priv->first_bgd, dev, priv);
    block_group_desc_t *bg = (block_group_desc_t *) root_buf;
    for (int i = 0; i < priv->number_of_bgs; i++) {
        if (bg->num_of_unalloc_inode) {
            // If the bg has some unallocated inodes,
            // find which inode is unallocated
            // This is easy:
            // For each bg we have sb->inodes_in_blockgroup inodes,
            // this one has num_of_unalloc_inode inodes unallocated,
            // therefore the latest id is:

            *id = ((i + 1) * priv->sb.inodes_in_blockgroup) - bg->num_of_unalloc_inode + 1;
            bg->num_of_unalloc_inode--;
            ext2_write_block(root_buf, priv->first_bgd + i, dev, priv);
            // Now, update the superblock as well
            ext2_read_block(root_buf, priv->sb.superblock_id, dev, priv);
            struct Ext2Superblock *sb = (struct Ext2Superblock *) root_buf;
            sb->unallocatedinodes--;
            ext2_write_block(root_buf, priv->sb.superblock_id, dev, priv);
            return;
        }
        bg++;
    }
}
*/
/*
void ext2_alloc_block(u32 *out, struct Device *dev, ext2_priv_data *priv) {
    // Algorithm: Loop through block group descriptors,
    // find which bg has a kfree block
    // and set that.

    ext2_read_block(root_buf, priv->first_bgd, dev, priv);
    block_group_desc_t *bg = (block_group_desc_t *) root_buf;
    for (int i = 0; i < priv->number_of_bgs; i++) {
        if (bg->num_of_unalloc_block) {
            *out = priv->sb.blocks - bg->num_of_unalloc_block + 1;
            bg->num_of_unalloc_block--;
            ext2_write_block(root_buf, priv->first_bgd + i, dev, priv);

            kprintf("Allocated block %d\n", *out);

            ext2_read_block(root_buf, priv->sb.superblock_id, dev, priv);
            struct Ext2Superblock *sb = (struct Ext2Superblock *) root_buf;
            sb->unallocatedblocks--;
            ext2_write_block(root_buf, priv->sb.superblock_id, dev, priv);
            return;
        }
        bg++;
    }
}
*/
/*
u8 ext2_touch(char *file, struct Device *dev, ext2_priv_data *priv) {
    if (!dev->driver->write)
        return 0;
    // file = "/levex.txt";
    // First create the inode
    char *fil = (char *) kmalloc(strlen(file) + 1, 0, "newFil");
    memcpy(fil, file, strlen(file) + 1);
    struct Ext2Inode *fi = (struct Ext2Inode *) kmalloc(sizeof(struct Ext2Inode), 0, "newstruct Ext2Inode");
    fi->hardlinks = 1;
    fi->size = 0;
    fi->type = INODE_TYPE_FILE;
    fi->disk_sectors = 2;
    // Create the directory entry
    size_t n = strsplit(fil, '/');
    n--;
    while (n) {
        fil += strlen(fil) + 1;
        n--;
    }
    //kprintf("filename: %s\n", fil);
    ext2_dir *entry = (ext2_dir *) kmalloc(sizeof(ext2_dir) + strlen(fil) + 1, , 0, "newExt2_dir");
    entry->size = sizeof(ext2_dir) + strlen(fil) + 1;
    entry->namelength = strlen(fil) + 1;
    entry->reserved = 0;
    memcpy(&entry->reserved + 1, fil, strlen(fil) + 1);
    //kprintf("Length of dir entry: %d + %d = %d\n", sizeof(ext2_dir), strlen(fil), entry->size);
    // Now, calculate this inode's id,
    // this is done from the superblock's inodes field
    // don't forget to update the superblock as well.

    u32 id = 0;
    ext2_find_new_inode_id(&id, dev, priv);
    //kprintf("Inode id = %d\n", id);
    entry->inode = id;
    //ext2_read_directory(0, entry, dev, priv);
    // Find where the previous inode is
    // and put this inode after this

    u32 block = 0; // The block where this inode should be written
    u32 ioff = 0; // Offset into the block function to sizeof(inode_t)
    ext2_get_inode_block(id, &block, &ioff, dev, priv);
    //kprintf("This inode is located on block %d with ioff %d\n", block, ioff);
    // First, read the block in
    ext2_read_block(root_buf, block, dev, priv);
    struct Ext2Inode *winode = (struct Ext2Inode *) root_buf;
    for (int i = 0; i < ioff; i++)
        winode++;
    memcpy(winode, fi, sizeof(struct Ext2Inode));
    ext2_write_block(root_buf, block, dev, priv);
    // Now, we have added the inode, write the superblock as well.
    //ext2_write_block(&priv->sb, priv->sb.superblock_id, dev, priv);
    // Now, add the directory entry,
    // for this we have to locate the directory that holds us,
    // and find his inode.
    // We call ext2_find_file_inode() to place the inode to inode_buf

    char *f = (char *) kmalloc(strlen(file) + 1, 0, "newChar*");
    memcpy(f, file, strlen(file) + 1);
    str_backspace(f, '/');

    //kprintf("LF: %s\n", f);
    if (!inode) inode = (struct Ext2Inode *) kmalloc(sizeof(struct Ext2Inode), 0, "newstruct Ext2Inode");
    if (!block_buf) block_buf = (u8 *) kmalloc(priv->blocksize, 0, "newBlockBuf");
    u32 t = ext2_find_file_inode(f, inode, dev, priv);
    t++;
    //kprintf("Parent is inode %d\n", t);
    u8 found = 0;
    for (int i = 0; i < 12; i++) {
        // Loop through the dpb to find an empty spot
        if (inode->dbp[i] == 0) {
            // This means we have not yet found a place for our entry,
            // and the inode has no block allocated.
            // Allocate a new block for this inode and place it there.

            u32 theblock = 0;
            ext2_alloc_block(&theblock, dev, priv);
            inode->dbp[i] = theblock;
            ext2_write_inode(inode, t, dev, priv);
        }
        // This DBP points to an array of directory entries
        ext2_read_block(block_buf, inode->dbp[i], dev, priv);
        // Loop throught the directory entries
        ext2_dir *d = (ext2_dir *) block_buf;
        u32 passed = 0;
        while (d->inode != 0) {
            if (d->size == 0) break;
            u32 truesize = d->namelength + 8;
            //kprintf("true size has modulo 4 of %d, adding %d\n", truesize % 4, 4 - truesize%4);
            truesize += 4 - truesize % 4;
            u32 origsize = d->size;
            //kprintf("Truesize: %d Origsize: %d\n", truesize, origsize);
            if (truesize != d->size) {
                // This is the last entry. Adjust the size to make space for our
                // ext2_dir! Also, note that according to ext2-doc, entries must be on
                // 4 byte boundaries!

                d->size = truesize;
                //kprintf("Adjusted entry len:%d, name %s!\n", d->size, &d->reserved + 1);
                // Now, skip to the next
                passed += d->size;
                d = (ext2_dir *) ((u32) d + d->size);
                // Adjust size
                entry->size = priv->blocksize - passed;
                //kprintf("Entry size is now %d\n", entry->size);
                break;
            }
            //kprintf("skipped len: %d, name:%s!\n", d->size, &d->reserved + 1);
            passed += d->size;
            d = (ext2_dir *) ((u32) d + d->size);
        }
        // There is a problem, however. The last entry will always span the whole
        // block. We have to check if its size field is bigger than what it really is.
        // If it is, adjust its size, and add the entry after it, adjust our size
        // to span the block fully. If not, continue as we did before to the next DBP.

        if (passed >= priv->blocksize) {
            //kprintf("Couldn't find it in DBP %d (%d > %d)\n", i, passed, priv->blocksize);
            continue;
        }
        // Well, found a kfree entry!
        //d = (ext2_dir *)((u32)d + d->size);
        dir_write:
        memcpy(d, entry, entry->size);
        ext2_write_block(block_buf, inode->dbp[i], dev, priv);
        //kprintf("Wrote to %d\n", inode->dbp[i]);
        return 1;
    }
    //kprintf("Couldn't write.\n");
    return 0;
}
*/
/*
u8 ext2_writefile(char *fn, char *buf, u32 len, struct Device *dev, ext2_priv_data *priv) {
    // Steps to write to a file:
    // - Locate and load the inode
    // - Check if it is of type INODE_TYPE_FILE
    // -- If no, bail out.
    // - If inode->size == 0
    // -- Allocate len / priv->blocksize amount of blocks.
    // --- Write the buf to the blocks.
    // - Else, check which block has the last byte, by
    //   dividing inode->size by priv->blocksize.
    // -- Load that block.
    // -- Inside, the last byte is (inode->size)%priv->blocksize
    // -- If len < priv->blocksize - (inode->size)%priv->blocksize
    //    which means that the buf can fill the block.
    // --- Write and return noerror.
    // -- Else,
    // --- Write the maximum possible bytes to the block.
    // --- The next block doesn't exist. Allocate a new one.
    // --- Write the rest to that block and repeat.
    // ALSO, on write: adjust inode->size !!!


    // Locate and load the inode
    u32 inode_id = ext2_find_file_inode(fn, inode, dev, priv);
    inode_id++;
    if (inode_id == 1) return 0;
    kprintf("%s's inode is %d\n", fn, inode_id);
    if (!inode) inode = (struct Ext2Inode *) kmalloc(sizeof(struct Ext2Inode), 0, "newstruct Ext2Inode");
    ext2_read_inode(inode, inode_id, dev, priv);
    // Check if it is of type INODE_TYPE_FILE
    *if(! (inode->type & INODE_TYPE_FILE))
    {
        // -- If no, bail out.
        kprintf("Not a file.\n");
        return 0;
    }
    // If inode->size == 0
    if (inode->size == 0) {
        // Allocate len / priv->blocksize amount of blocks.
        u32 blocks_to_alloc = len / priv->blocksize;
        blocks_to_alloc++; // Allocate atleast one!
        if (blocks_to_alloc > 12) {
            // @todo
            kprintf("Sorry, can't write to files bigger than 12Kb :(\n");
            return 0;
        }
        for (int i = 0; i < blocks_to_alloc; i++) {
            u32 bid = 0;
            ext2_alloc_block(&bid, dev, priv);
            inode->dbp[i] = bid;
            //kprintf("Set dbp[%d] to %d\n", i, inode->dbp[i]);
        }
        kprintf("Allocated %d blocks!\n", blocks_to_alloc);
        inode->size += len; // UPDATE the size
        // Commit the inode to the disk
        ext2_write_inode(inode, inode_id - 1, dev, priv);
        // Write the buf to the blocks.
        for (int i = 0; i < blocks_to_alloc; i++) {
            // We loop through the blocks and write.
            ext2_read_block(root_buf, inode->dbp[i], dev, priv);
            if (i + 1 < blocks_to_alloc) { // If not last block
                memcpy(root_buf, buf + i * priv->blocksize, priv->blocksize);
            } else {// If last block
                kprintf("Last block write %d => %d!\n", i, inode->dbp[i]);
                memcpy(root_buf, buf + i * priv->blocksize, len);
            }
            ext2_write_block(root_buf, inode->dbp[i], dev, priv);
        }
        kprintf("Wrote the data to fresh blocks!\n");
        return 1;
    }
    // Else, check which block has the last byte, by
    // dividing inode->size by priv->blocksize.

    u32 last_data_block = inode->size / priv->blocksize;
    u32 last_data_offset = (inode->size) % priv->blocksize;
    // Load that block.
    ext2_read_block(root_buf, last_data_block, dev, priv);
    // If len < priv->blocksize - (inode->size)%priv->blocksize

    if (len < priv->blocksize - last_data_offset) {
        //    which means that the buf can fill the block.
        // Write and return noerror.
        memcpy(root_buf + last_data_offset, buf, len);
        ext2_write_block(root_buf, last_data_block, dev, priv);
        return 1;
    }
    //Else,
    // --- Write the maximum possible bytes to the block.
    // --- The next block doesn't exist. Allocate a new one.
    // --- Write the rest to that block and repeat.

    // u32 data_wrote = 0;
    // memcpy(root_buf + last_data_offset, buf, priv->blocksize - last_data_offset);
    // data_wrote += priv->blocksize - last_data_offset;

    return 0;
}
 */
/*
u8 ext2_exist(char *file, struct Device *dev, ext2_priv_data *priv) {
    return ext2_read_file(file, 0, dev, priv);
}
*/

static int ext2_read_singly_linked(u32 singlyBlockId, u32 blockid, void *buf, struct Ext2PrivData *priv) {
    u32 *singlyBlock = kmalloc(priv->blocksize, 0, "singlyBlock");
    if (singlyBlock == NULL)
        return -1;

    ext2_read_block(singlyBlock, singlyBlockId, priv);
    ext2_read_block(buf, singlyBlock[blockid], priv);

    kfree(singlyBlock);
    return 0;
}

#define SIZE_OF_SINGLY (priv->blocksize * priv->blocksize / 4)

static int ext2_read_doubly_linked(u32 doublyblockid, u32 blockid, void *buf, struct Ext2PrivData *priv) {

    u32 *doublyBlock = kmalloc(priv->blocksize, 0, "doublyBlock");
    if (doublyBlock == NULL)
        return -1;

    ext2_read_block(doublyBlock, doublyblockid, priv);
    ext2_read_singly_linked(doublyBlock[blockid / SIZE_OF_SINGLY], blockid % SIZE_OF_SINGLY, buf, priv);

    kfree(doublyBlock);
    return 0;
}

#define SIZE_OF_DOUBLY (SIZE_OF_SINGLY * priv->blocksize)

static int ext2_read_triply_linked(u32 triplyblockid, u32 blockid, void *buf, struct Ext2PrivData *priv) {

    u32 *triplyBlock = kmalloc(priv->blocksize, 0, "triplyBlock");
    if (triplyBlock == NULL)
        return -1;

    ext2_read_block(triplyBlock, triplyblockid, priv);
    ext2_read_doubly_linked(triplyBlock[blockid / SIZE_OF_DOUBLY], blockid % SIZE_OF_DOUBLY, buf, priv);

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
            kSerialPrintf("[ext2] readblock: block %d outside range (max: %d)!\n", b, priv->sb.blocks);
            return -1;
        }

        LOG("Reading block: %d\n", b);
        ext2_read_block((void *) buffer, b, priv);
    } else if (blocknum - 12 < (priv->blocksize / sizeof(u32))) {
        ext2_read_singly_linked(pathInode->singly_block, blocknum - 12, buffer, priv);
    } else if (blocknum - 12 - (priv->blocksize / sizeof(u32)) < SIZE_OF_SINGLY) {
        u32 tmp = blocknum - 12 - (priv->blocksize / sizeof(u32));
        ext2_read_doubly_linked(pathInode->doubly_block, tmp, buffer, priv);
    } else {
        u32 tmp = blocknum - 12 - (priv->blocksize / sizeof(u32)) - SIZE_OF_SINGLY;
        ext2_read_triply_linked(pathInode->triply_block, tmp, buffer, priv);
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

    ext2_read_block(buf, b, priv);
    struct Ext2DirEntry *dir = (struct Ext2DirEntry *) (buf + pathInode->tmpDir);

    memcpy(result->d_name, &dir->reserved + 1, dir->namelength);
    result->d_name[dir->namelength] = 0;

    result->d_ino = dir->inode;
    result->d_type = FT_FILE;

    LOG("dir size = %u, %X\n", dir->size, dir);
    pathInode->tmpDir += dir->size;

    dir = (struct Ext2DirEntry *) ((u32) dir + dir->size);
    if ((u32) pathInode->tmpDir >= priv->blocksize || dir->inode == 0) {
        LOG("bite en bois\n");
        pathInode->tmpDir = 0;
        pathInode->tmpBreadir += 1;
    }

    kfree(buf);
    return result;
}

static u32 ext2_read_directory(const char *filename, struct Ext2DirEntry *dir) {
    while (dir->inode != 0) {
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
            kSerialPrintf("error 1");
            goto ext2LookupFaillure;
        }

        ext2_read_block(buf, b, priv);
        fileInode = ext2_read_directory(name, buf);
        if (fileInode)
            break;
    }

    if (!fileInode)
        goto ext2LookupFaillure;

    inode = kmalloc(sizeof(struct Ext2Inode), 0, "newInode");
    if (inode == NULL)
        goto ext2LookupFaillure;

    LOG("Found inode %s! %d\n", name, fileInode);
    ext2_read_inode(inode, fileInode, priv);

    struct FsPath *newPath = kmalloc(sizeof(struct FsPath), 0, "newExt2FsPath");
    if (newPath == NULL)
        goto ext2LookupFaillure;

    inode->tmpBreadir = 0;
    inode->tmpDir = 0;

    newPath->privateData = inode;
    newPath->size = inode->size;

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

    ext2_read_inode(rootInode, 2, priv);
    if ((rootInode->type & 0xF000) != INODE_TYPE_DIRECTORY) {
        kSerialPrintf("FATAL: Root directory is not a directory! (%X)\n", (rootInode->type & 0xF000));
        goto ext2RootFaillure;
    }

    struct FsPath *rootPath = kmalloc(sizeof(struct FsPath), 0, "ext2Root");
    if (rootPath == NULL)
        goto ext2RootFaillure;

    rootPath->privateData = rootInode;
    rootPath->size = 0;
    return rootPath;

    ext2RootFaillure:
    kSerialPrintf("[ext2] get root dir faillure\n");
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
        kSerialPrintf("Invalid EXT2 signature, have: 0x%x!\n", sb->ext2_sig);
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
    kSerialPrintf("[ext2] failure\n");
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