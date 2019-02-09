//
// Created by rebut_p on 09/02/19.
//

#ifndef KERNEL_PIPE_H
#define KERNEL_PIPE_H

#include <k/ktypes.h>
#include <sys/mutex.h>

struct Pipe {
    char *buffer;
    int read_pos;
    int write_pos;

    u32 refcount;
    struct Task *task;
    struct Mutex mtx;
};

struct Pipe *pipeCreate();
struct Pipe *pipeAddref(struct Pipe *pipe);
int pipeRead(struct Pipe *pipe, char *buffer, u32 size);
int pipeWrite(struct Pipe *pipe, const char *buffer, u32 size);
void pipeDelete(struct Pipe *pipe);


#endif //KERNEL_PIPE_H
