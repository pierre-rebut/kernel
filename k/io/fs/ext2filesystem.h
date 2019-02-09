//
// Created by rebut_p on 26/12/18.
//

#ifndef KERNEL_EXT2FILESYSTEM_H
#define KERNEL_EXT2FILESYSTEM_H

#include <k/ktypes.h>
#include <io/device/device.h>

#define EXT2_SIGNATURE 0xEF53

#define INODE_TYPE_FIFO 0x1000
#define INODE_TYPE_CHAR_DEV 0x2000
#define INODE_TYPE_DIRECTORY 0x4000
#define INODE_TYPE_BLOCK_DEV 0x6000
#define INODE_TYPE_FILE 0x8000
#define INODE_TYPE_SYMLINK 0xA000
#define INODE_TYPE_SOCKET 0xC000

struct Ext2Superblock {
    u32 inodes;
    u32 blocks;
    u32 reserved_for_root;
    u32 unallocatedblocks;
    u32 unallocatedinodes;
    u32 superblock_id;
    u32 blocksize_hint; // shift by 1024 to the left
    u32 fragmentsize_hint; // shift by 1024 to left
    u32 blocks_in_blockgroup;
    u32 frags_in_blockgroup;
    u32 inodes_in_blockgroup;
    u32 last_mount;
    u32 last_write;
    u16 mounts_since_last_check;
    u16 max_mounts_since_last_check;
    u16 ext2_sig; // 0xEF53
    u16 state;
    u16 op_on_err;
    u16 minor_version;
    u32 last_check;
    u32 max_time_in_checks;
    u32 os_id;
    u32 major_version;
    u16 uuid;
    u16 gid;
    u32 first_nonreserved_inode;
    u16 inode_size;
    u8 unused[934];
} __attribute__((packed));

struct Ext2BlockGroupDesc {
    u32 block_of_block_usage_bitmap;
    u32 block_of_inode_usage_bitmap;
    u32 block_of_inode_table;
    u16 num_of_unalloc_block;
    u16 num_of_unalloc_inode;
    u16 num_of_dirs;
    u8 unused[14];
} __attribute__((packed));

struct Ext2Inode {
    u16 type;
    u16 uid;
    u32 size;
    u32 last_access;
    u32 create_time;
    u32 last_modif;
    u32 delete_time;
    u16 gid;
    u16 hardlinks;
    u32 disk_sectors;
    u32 flags;
    u32 ossv1;
    u32 dbp[12];
    u32 singly_block;
    u32 doubly_block;
    u32 triply_block;
    u32 gen_no;
    u32 reserved1;
    u32 reserved2;
    u32 fragment_block;
    u32 tmpBreadir;
    u32 tmpDir;
    u8 ossv2[4];
} __attribute__((packed));

struct Ext2DirEntry {
    u32 inode;
    u16 size;
    u8 namelength;
    u8 reserved;
    /* name here */
} __attribute__((packed));

struct Ext2PrivData {
    struct Ext2Superblock sb;
    u32 first_bgd;
    u32 number_of_bgs;
    u32 blocksize;
    u32 sectors_per_block;
    u32 inodes_per_block;
    struct Device *device;
};

void initExt2FileSystem();

#endif //KERNEL_EXT2FILESYSTEM_H
