//
// Created by rebut_p on 10/02/19.
//

#include <sys/paging.h>
#include <include/stdio.h>
#include <include/string.h>
#include <sys/console.h>
#include <io/terminal.h>
#include "tty.h"
#include "sheduler.h"

static const char *cmdline;
static const char **args;
static const char **env;

void ttyTaskLoop() {
    while (1) {
        clearTerminal(consoleGetActiveConsole()->tty);
        u32 pid = createProcess(cmdline, args, env);
        taskWaitEvent(TaskEventWaitPid, pid);
        kprintf("Resetting terminal (kill: %u)\n", pid);
        taskWaitEvent(TaskEventTimer, 1000);
    }
}

void initTTY(const char *c, const char **av, const char **e) {
    cmdline = c;
    args = av;
    env = e;
}

void createNewTTY() {
    struct TaskCreator taskInfo = {
            .type = T_PROCESS,
            .pageDirectory = kernelPageDirectory,
            .entryPoint = (u32) &ttyTaskLoop,
            .privilege = TaskPrivilegeKernel,
            .ac = 0,
            .av = NULL,
            .env = NULL,
            .cmdline = strdup("tty"),
            .parent = &kernelTask
    };

    struct Task *task = createTask(&taskInfo);
    if (!task)
        return;

    task->currentDir = currentTask->currentDir;
    task->currentDir->refcount += 1;

    schedulerAddActiveTask(task);
}