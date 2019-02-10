//
// Created by rebut_p on 09/02/19.
//

#include <sys/allocator.h>
#include <task.h>
#include <include/kstdio.h>
#include "pipe.h"

#define PIPE_SIZE 1024

struct Pipe *pipeCreate() {
    struct Pipe *pipe = kmalloc(sizeof(struct Pipe), 0, "newPipe");
    if (pipe == NULL)
        return NULL;

    pipe->buffer = kmalloc(sizeof(char) * PIPE_SIZE, 0, "newPipe");
    if (pipe->buffer == NULL) {
        kfree(pipe);
        return NULL;
    }

    pipe->read_pos = 0;
    pipe->write_pos = 0;
    pipe->refcount = 0;
    pipe->task = NULL;
    mutexReset(&pipe->mtx);
    return pipe;
}

struct Pipe *pipeAddref(struct Pipe *pipe) {
    pipe->refcount += 1;
    return pipe;
}

int pipeRead(struct Pipe *pipe, char *buffer, u32 size) {
    if (!pipe || !buffer)
        return -1;

    mutexLock(&pipe->mtx);
    if (pipe->write_pos == pipe->read_pos) {
        pipe->task = currentTask;
        mutexUnlock(&pipe->mtx);
        taskWaitEvent(TaskEventPipe, 0);
        mutexLock(&pipe->mtx);
    }

    u32 read = 0;
    while(read < size && pipe->read_pos != pipe->write_pos) {
        buffer[read] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_SIZE;
        read++;
    }

    if (pipe->task) {
        pipe->task->event.type = TaskEventNone;
        pipe->task = NULL;
    }
    mutexUnlock(&pipe->mtx);
    return (int) read;
}

int pipeWrite(struct Pipe *pipe, const char *buffer, u32 size) {
    if (!pipe || !buffer)
        return -1;

    klog("here i am now\n");

    mutexLock(&pipe->mtx);
    for (u32 written = 0; written < size; written++) {

        if ((pipe->write_pos + 1) % PIPE_SIZE == pipe->read_pos) {
            pipe->task = currentTask;
            mutexUnlock(&pipe->mtx);
            taskWaitEvent(TaskEventPipe, 0);
            mutexLock(&pipe->mtx);
            pipe->task = NULL;
        }

        pipe->buffer[pipe->write_pos] = buffer[written];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_SIZE;

        if (pipe->task != NULL) {
            pipe->task->event.type = TaskEventNone;
            pipe->task = NULL;
        }
    }

    mutexUnlock(&pipe->mtx);
    return size;
}

void pipeDelete(struct Pipe *pipe) {
    if (pipe == NULL)
        return;

    pipe->refcount -= 1;
    if (pipe->refcount != 0)
        return;

    kfree(pipe->buffer);
    kfree(pipe);
}