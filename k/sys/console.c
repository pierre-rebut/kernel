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

static struct Console *consoleLists = NULL;
struct Console *activeConsole = NULL;
struct Console *kernelConsole = NULL;

void initConsole() {
    currentKeyMap = keyMap;
    kernelConsole = createConsole();
    activeConsole = kernelConsole;
}

struct Console *createConsole() {
    struct Console *newConsole = kmalloc(sizeof(struct Console), 0, "newConsole");
    if (!newConsole)
        return NULL;

    if (consoleLists)
        consoleLists->next = newConsole;

    newConsole->readBuffer.readPtr = 0;
    newConsole->readBuffer.writePtr = 0;

    newConsole->mode = ConsoleModeText;
    newConsole->prev = consoleLists;
    newConsole->next = NULL;
    consoleLists = newConsole;
    return newConsole;
}

void destroyConsole(struct Console *console) {
    if (console == kernelConsole)
        return;

    if (console == activeConsole)
        setActiveConsole(console->prev);

    struct Console *tmp = consoleLists;

    while (tmp != NULL) {
        if (tmp == console) {
            if (tmp->next)
                tmp->next->prev = tmp->prev;
            else
                consoleLists = tmp->prev;
            if (tmp->prev)
                tmp->prev->next = tmp->next;
            return;
        }
        tmp = tmp->prev;
    }
}

void setActiveConsole(struct Console *console) {
    if (console == activeConsole)
        return;

    switchVgaMode(console->mode);
    activeConsole = console;
}

void consoleKeyboardHandler(int code) {
    LOG("keyboard: %d\n", code);

    int release = code >> 7;
    code &= ~(1 << 7);

    if (code == 91 && !release) {
        taskKill(activeConsole->task);
        return;
    }

    if (code == 42 || code == 54) {
        currentKeyMap = (release ? keyMap : keyMapShift);
        return;
    }

    if (release)
        return;

    struct CirBuffer *val = &activeConsole->readBuffer;

    if ((val->writePtr + 1) % CONSOLE_BUFFER_SIZE == val->readPtr)
        val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;

    char c = currentKeyMap[code - 2];
    val->buffer[val->writePtr] = c;
    val->writePtr = (val->writePtr + 1) % CONSOLE_BUFFER_SIZE;

    if (activeConsole->mode == ConsoleModeText) {
        if (c == 0)
            writeStringTerminal("^@", 2);
        else
            writeTerminal(c);
    }

    if (isConsoleReadReady(activeConsole) == 1)
        activeConsole->task->event.type = TaskEventNone;
}

char isConsoleReadReady(struct Console *console) {
    struct CirBuffer *val = &console->readBuffer;

    int tmp = val->readPtr;
    while (tmp != val->writePtr) {
        if (val->buffer[tmp] == '\n')
            return 1;
        tmp = (tmp + 1) % CONSOLE_BUFFER_SIZE;
    }
    return 0;
}

char consoleGetkey(struct Console *console) {
    struct CirBuffer *val = &console->readBuffer;

    if (val->readPtr == val->writePtr)
        return -1;

    char tmp = val->buffer[val->readPtr];
    val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;
    return tmp;
}

s32 readKeyboardFromConsole(void *entryData, void *buf, u32 size) {
    taskWaitEvent(TaskEventKeyboard, 0);

    LOG("keyboard event end\n");

    struct Console *console = (struct Console *) entryData;

    char *str = (char *) buf;
    u32 read = 0;
    char key;

    while ((key = consoleGetkey(console)) != -1 && read < size) {
        str[read] = key;
        read++;
    }

    return read;

}