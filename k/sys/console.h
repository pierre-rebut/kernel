//
// Created by rebut_p on 16/12/18.
//

#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include "task.h"
#include <sys/mutex.h>

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
    enum ConsoleMode mode;
    struct CirBuffer readBuffer;

    void *videoBuffer;
    struct TerminalBuffer *tty;

    struct Task *readingTask;
    struct Mutex mtx;
};

void initConsole();

int consoleSwitchById(int id);

struct Console *consoleGetActiveConsole();

void consoleKeyboardHandler(int code);

char consoleGetkey(struct Console *console);

int consoleGetkey2(struct Console *console);

s32 consoleReadKeyboard(void *entryData, void *buf, u32 size);

s32 consoleWriteStandard(void *entryData, const char *buf, u32 size);

s32 consoleForceWrite(const char *buf, u32 size);

#endif //KERNEL_CONSOLE_H
