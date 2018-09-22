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
    size_t i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i])
            return -1;

        i++;
    }
    if (a[i] != b[i])
        return -1;
    return 0;
}

int open(const char *pathname, int flags) {
    if (kfs == NULL || pathname == NULL)
        return -1;

    struct kfs_inode *node = (struct kfs_inode *) ((void *) kfs + (kfs->inode_idx * KFS_BLK_SZ));

    u32 i;
    for (i = 0; i < kfs->inode_cnt; i++) {
        if (strcmp(node->filename, pathname) == 0)
            break;
        node = (struct kfs_inode *) ((void *) kfs + (node->next_inode * KFS_BLK_SZ));
    }

    if (i == kfs->inode_cnt)
        return -2;

    if (kfs_checksum(node, sizeof(struct kfs_inode) - 4) != node->cksum)
        return -3;

    for (int i = 0; i < 256; i++) {
        if (fdTable[i].used == 0) {
            fdTable[i].used = 1;
            fdTable[i].flags = flags;
            fdTable[i].offset = 0;
            fdTable[i].node = node;
            return i;
        }
    }
    return -4;
}

ssize_t read(int fd, void *buf, size_t size) {
    if (fd < 0 || fd > 255 || kfs == NULL)
        return 0;

    if (fdTable[fd].used == 0)
        return 0;

    return 0;
}

off_t seek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd > 255 || kfs == NULL)
        return 0;

    return 0;
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

        printf("%d - file: %s, size: %d\n", node->inumber, node->filename, node->file_sz);
        node = (struct kfs_inode *) ((void *) kfs + (node->next_inode * KFS_BLK_SZ));
    }
}