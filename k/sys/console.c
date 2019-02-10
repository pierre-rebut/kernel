//
// Created by rebut_p on 16/12/18.
//

#include <kstdio.h>
#include <sys/allocator.h>
#include <io/libvga.h>
#include <io/terminal.h>
#include <tty.h>
#include <string.h>
#include "console.h"

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

static char *currentKeyMap = NULL;
static char keyMap[] = "&\0\"'(-\0_\0\0)=\0\tazertyuiop^$\n\0qsdfghjklm\0\0\0*wxcvbn,;:!\0\0\0 ";
static char keyMapShift[] = "1234567890\0+\0\tAZERTYUIOP\0\0\n\0QSDFGHJKLM%\0\0\0WXCVBN?./\0";

static struct Console *consoleLists[12] = {0};
static int activeConsoleId = -1;

static struct Console *createConsole();

void initConsole() {
    currentKeyMap = keyMap;
    consoleSwitchById(0);
}

static struct Console *createConsole() {
    struct Console *newConsole = kmalloc(sizeof(struct Console), 0, "newConsole");
    if (!newConsole)
        return NULL;

    newConsole->readBuffer.readPtr = 0;
    newConsole->readBuffer.writePtr = 0;

    newConsole->mode = ConsoleModeText;
    newConsole->videoBuffer = NULL;
    newConsole->tty = createTerminal();

    mutexReset(&newConsole->mtx);
    newConsole->readingTask = NULL;
    newConsole->activeProcess = NULL;

    return newConsole;
}

int consoleSwitchById(int id) {
    if (id < 0 || id > 11)
        return -1;

    if (id == activeConsoleId)
        return 0;

    if (consoleLists[id] == NULL)
        consoleLists[id] = createConsole();

    struct Console *cons = consoleLists[id];

    switchVgaMode(cons->mode);

    if (cons->mode == ConsoleModeText)
        updateTerminal(cons->tty);
    else
        setVgaFrameBuffer(cons->videoBuffer);

    activeConsoleId = id;
    return 0;
}

struct Console *consoleGetActiveConsole() {
    return consoleLists[activeConsoleId];
}

char consoleGetLastkey(struct Console *console) {
    struct CirBuffer *val = &console->readBuffer;

    if (val->readPtr == val->writePtr)
        return -1;

    if (val->writePtr == 0)
        val->writePtr = CONSOLE_BUFFER_SIZE - 1;
    else
        val->writePtr -= 1;
    return val->buffer[val->writePtr];
}

void consoleKeyboardHandler(int code) {
    LOG("keyboard: %d\n", code);

    int release = code >> 7;
    code &= ~(1 << 7);

    struct Console *cons = consoleLists[activeConsoleId];

    if (code == 91 && !release && cons->activeProcess) {
        taskKill(cons->activeProcess);
        return;
    } else if (code >= 59 && code <= 68 && !release) {
        int consId = code - 59;
        char isNotReady = (consoleLists[consId] == NULL);
        consoleSwitchById(consId);
        if (isNotReady)
            createNewTTY();

        return;
    } else if (code == 14 && !release) {
        char c = consoleGetLastkey(cons);
        if (c != -1)
            terminalRemoveLastChar(cons->tty, cons->mode == ConsoleModeText);

        return;
    }

    if (code == 42 || code == 54) {
        currentKeyMap = (release ? keyMap : keyMapShift);
        return;
    }

    char c = currentKeyMap[code - 2];
    if (release)
        return;

    struct CirBuffer *val = &cons->readBuffer;

    if ((val->writePtr + 1) % CONSOLE_BUFFER_SIZE == val->readPtr)
        val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;

    val->buffer[val->writePtr] = c;
    val->tmpBuffer[val->writePtr] = code;
    val->writePtr = (val->writePtr + 1) % CONSOLE_BUFFER_SIZE;

    if (cons->mode == ConsoleModeText) {
        if (c == 0)
            terminalPutchar(cons->tty, 1, '@');
        else
            terminalPutchar(cons->tty, 1, c);
        terminalUpdateCursor(cons->tty);
    }

    if (c == '\n')
        taskResetEvent(cons->readingTask);
}

char consoleGetkey(struct Console *console) {
    struct CirBuffer *val = &console->readBuffer;

    if (val->readPtr == val->writePtr)
        return -1;

    char tmp = val->buffer[val->readPtr];
    val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;
    return tmp;
}

int consoleGetkey2(struct Console *console) {
    struct CirBuffer *val = &console->readBuffer;

    if (val->readPtr == val->writePtr)
        return -1;

    int tmp = val->tmpBuffer[val->readPtr];
    val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;
    return tmp;
}

static u32 consoleInternalRead(struct Console *console, char *str, u32 size) {
    u32 read = 0;
    char key;

    while (read < size && (key = consoleGetkey(console)) != -1) {
        str[read] = key;
        read++;
    }

    console->readingTask = NULL;
    return read;
}

static char isConsoleReadReady(struct Console *console) {
    struct CirBuffer *val = &console->readBuffer;

    int tmp = val->readPtr;
    while (tmp != val->writePtr) {
        if (val->buffer[tmp] == '\n')
            return 1;
        tmp = (tmp + 1) % CONSOLE_BUFFER_SIZE;
    }
    return 0;
}

s32 consoleReadKeyboard(void *entryData, void *buf, u32 size) {
    struct Console *console = (struct Console *) entryData;
    s32 ret;

    mutexLock(&console->mtx);
    if (isConsoleReadReady(console)) {
        ret = consoleInternalRead(console, buf, size);
        mutexUnlock(&console->mtx);
        return ret;
    }

    console->readingTask = currentTask;
    taskWaitEvent(TaskEventKeyboard, 0);

    LOG("keyboard event end\n");
    ret = consoleInternalRead(console, buf, size);
    mutexUnlock(&console->mtx);

    klog("pute de train: [%s]\n", buf);
    return ret;
}

s32 consoleWriteStandard(void *entryData, const char *buf, u32 size) {
    struct Console *cons = entryData;
    char writing = (cons->mode == ConsoleModeText && consoleLists[activeConsoleId] == cons);

    for (u32 i = 0; i < size; i++) {
        terminalPutchar(cons->tty, writing, buf[i]);
    }

    if (writing)
        terminalUpdateCursor(cons->tty);
    return size;
}

s32 consoleForceWrite(const char *buf, u32 size) {
    return consoleWriteStandard(consoleLists[activeConsoleId], buf, size);
}

int consoleSwitchVideoMode(struct Console *console, enum ConsoleMode mode) {
    if (console->mode == mode)
        return 0;

    console->mode = mode;

    if (mode == ConsoleModeVideo && console->videoBuffer == NULL)
        console->videoBuffer = kmalloc(VGA_VIDEO_WIDTH * VGA_VIDEO_HEIGHT, 0, "videobuffer");

    if (console == consoleLists[activeConsoleId])
        switchVgaMode(mode);
    return 0;
}

int consoleSetVgaFrameBuffer(struct Console *console, const void *buffer) {
    if (console->mode != ConsoleModeVideo)
        return -1;

    memcpy(console->videoBuffer, buffer, VGA_VIDEO_HEIGHT * VGA_VIDEO_WIDTH);
    if (console == consoleLists[activeConsoleId])
        setVgaFrameBuffer(buffer);
    return 0;
}