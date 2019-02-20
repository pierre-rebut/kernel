//
// Created by rebut_p on 17/02/19.
//

#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

enum ProcPathType
{
    PP_INFO,
    PP_PROC,
    PP_FOLDER
};

struct ProcPath
{
    enum ProcPathType type;
    int data;
    char *name;
};

int procRead(struct ProcPath *proc, void *buffer, int size, int offset);

#endif //KERNEL_PROC_H
