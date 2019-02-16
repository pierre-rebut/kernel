//
// Created by rebut_p on 17/02/19.
//

#include <kstd.h>
#include <task.h>
#include <kstdio.h>
#include <sys/allocator.h>
#include <sys/physical-memory.h>
#include <sys/time.h>
#include <io/pit.h>

#include "proc.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static int procReadProcess(pid_t pid, void *buffer, u32 size) {
    struct Task *task = getTaskByPid(pid);
    if (!task)
        return -1;

    return snprintf(buffer, size, "pid:%u\ncmdline:%s\nprivilege:%s\nevent:%d,%lu,%u\n",
                   task->pid,
                   (task->cmdline ? task->cmdline : "NONE"),
                   (task->privilege == TaskPrivilegeKernel ? "KERNEL" : "USER"),
                   task->event.type, task->event.timer, task->event.arg
    );
}

static int procReadInfo(int data, char *buffer, u32 size) {
    int read;
    switch (data) {
        case 0:
            read = snprintf(buffer, size, "%s,%u,%u\n", fsRootVolume->fs->name, fsRootVolume->refcount,
                           fsRootVolume->blockSize);
            struct FsMountVolume *tmpMntVolume = fsMountedVolumeList;
            while (tmpMntVolume) {
                struct FsVolume *tmpVolume = tmpMntVolume->mountedVolume;
                read += snprintf(buffer + read, size, "%s,%u,%u\n",
                                tmpVolume->fs->name, tmpVolume->refcount, tmpVolume->blockSize
                );
                tmpMntVolume = tmpMntVolume->next;
            }
            break;
        case 1: {
            u32 total, used;
            kmallocGetInfo(&total, &used);
            read = snprintf(buffer, size, "[PHYSMEM]\nused:%u\ntotal:%u\n\n[KERNEL MEM]\nused:%u\npaged:%u\n",
                           getTotalUsedPhysMemory(), getTotalPhysMemory(), total, used);
            break;
        }
        case 2:
            read = getCurrentDateAndTime(buffer, size);
            buffer[read++] = '\n';
            buffer[read] = '\0';
            break;
        case 3:
            read = snprintf(buffer, size, "%lu\n", gettick());
            break;
        case 4:
            read = snprintf(buffer, size, "pid:%u\ngid:%u\ncmdline:%s\nprivilege:%s\nevent:%d,%lu,%u\n",
                           currentTask->pid,
                           (currentTask->parent ? currentTask->parent->pid : 0),
                           (currentTask->cmdline ? currentTask->cmdline : "NONE"),
                           (currentTask->privilege == TaskPrivilegeKernel ? "KERNEL" : "USER"),
                           currentTask->event.type, currentTask->event.timer, currentTask->event.arg
            );
            break;
        case 5:
            read = snprintf(buffer, size, "K-OS version 1.0-arch\n");
            break;
        default:
            return -1;
    }
    return read;
}

int procRead(struct ProcPath *proc, void *buffer, int size, int offset) {
    (void) offset;

    LOG("[proc] readblock: %u\n", blocknum);
    if (!proc)
        return -1;

    switch (proc->type) {
        case PP_PROC:
            return procReadProcess(proc->data, buffer, (u32) size);
        case PP_INFO:
            return procReadInfo(proc->data, buffer, (u32) size);
        default:
            return -1;
    }
}