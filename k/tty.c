//
// Created by rebut_p on 10/02/19.
//

#include <sys/paging.h>
#include <include/kstdio.h>
#include <string.h>
#include <sys/console.h>
#include <io/terminal.h>
#include "tty.h"
#include "sheduler.h"

static const char *cmdline;
static const char **args;
static const char **env;

void ttyTaskLoop() {
    struct ExceveInfo execInfo = {
            .cmdline = cmdline,
            .env = env,
            .av = args,
            .fd_in = -1,
            .fd_out = -1
    };

    while (1) {
        clearTerminal(consoleGetActiveConsole()->tty);
        pid_t pid = createProcess(&execInfo);
        if (pid == -1)
            break;

        taskWaitEvent(TaskEventWaitPid, (u32) pid);
        kprintf("Resetting terminal (kill: %d)\n", pid);
        taskWaitEvent(TaskEventTimer, 1000);
    }

    kprintf("An error occured\n");
    taskExit();
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

    task->currentDir = kernelTask.currentDir;
    task->rootDir = kernelTask.rootDir;

    task->currentDir->refcount += 2;

    schedulerAddActiveTask(task);
}