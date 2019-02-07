//
// Created by rebut_p on 16/12/18.
//

#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include "task.h"

#define CONSOLE_BUFFER_SIZE 255

enum ConsoleMode {
    ConsoleModeVideo = 0,
    ConsoleModeText = 1,
};

struct CirBuffer {
    int readPtr;
    int writePtr;
    char buffer[CONSOLE_BUFFER_SIZE];
    int tmpBuffer[CONSOLE_BUFFER_SIZE];
};

struct Console {
    struct CirBuffer readBuffer;
    struct Task *task;
    enum ConsoleMode mode;

    struct Console *next;
    struct Console *prev;
};

extern struct Console *activeConsole;
extern struct Console *kernelConsole;

void initConsole();

struct Console *createConsole();

void destroyConsole(struct Console *console);

void setActiveConsole(struct Console *console);

void consoleKeyboardHandler(int code);

char isConsoleReadReady(struct Console *console);

char consoleGetkey(struct Console *console);

int consoleGetkey2(struct Console *console);

s32 readKeyboardFromConsole(void *entryData, void *buf, u32 size);

#endif //KERNEL_CONSOLE_H
