//
// Created by rebut_p on 16/12/18.
//

#include <stdio.h>
#include <sys/allocator.h>
#include <io/libvga.h>
#include <io/terminal.h>
#include "console.h"

//#define LOG(x, ...) kSerialPrintf((x), ##__VA_ARGS__)
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
    newConsole->task = NULL;

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

void consoleKeyboardHandler(int code) {
    LOG("keyboard: %d\n", code);

    int release = code >> 7;
    code &= ~(1 << 7);

    struct Console *cons = consoleLists[activeConsoleId];

    if (code == 91 && !release) {
        taskKill(cons->task);
        return;
    }

    if (code == 42 || code == 54) {
        currentKeyMap = (release ? keyMap : keyMapShift);
        return;
    }

    if (release)
        return;

    struct CirBuffer *val = &cons->readBuffer;

    if ((val->writePtr + 1) % CONSOLE_BUFFER_SIZE == val->readPtr)
        val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;

    char c = currentKeyMap[code - 2];
    val->buffer[val->writePtr] = c;
    val->tmpBuffer[val->writePtr] = code;
    val->writePtr = (val->writePtr + 1) % CONSOLE_BUFFER_SIZE;

    if (cons->mode == ConsoleModeText) {
        terminalPutchar(cons->tty, 1, c);
        terminalUpdateCursor(cons->tty);
    }

    if (c == '\n')
        taskResetEvent(cons->task);
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

s32 consoleReadKeyboard(void *entryData, void *buf, u32 size) {
    struct Console *console = (struct Console *) entryData;

    mutexLock(&console->mtx);
    console->task = currentTask;
    taskWaitEvent(TaskEventKeyboard, 0);

    LOG("keyboard event end\n");

    char *str = (char *) buf;
    u32 read = 0;
    char key;

    while ((key = consoleGetkey(console)) != -1 && read < size) {
        str[read] = key;
        read++;
    }

    console->task = NULL;
    mutexUnlock(&console->mtx);
    return read;
}

s32 consoleWriteStandard(void *entryData, const char *buf, u32 size) {
    struct Console *cons = entryData;
    char writing = (cons->mode == ConsoleModeText && consoleLists[activeConsoleId] == cons);

    for (u32 i = 0; i < size; i++) {
        terminalPutchar(cons->tty, writing, buf[i]);
    }

    terminalUpdateCursor(cons->tty);
    return size;
}

s32 consoleForceWrite(const char *buf, u32 size) {
    return consoleWriteStandard(consoleLists[activeConsoleId], buf, size);
}