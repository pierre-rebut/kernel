//
// Created by rebut_p on 16/12/18.
//

#include <system/allocator.h>
#include <io/libvga.h>
#include <io/terminal.h>
#include <tty.h>
#include <string.h>
#include <ascii.h>
#include <errno-base.h>

//#define LOG(x, ...) klog((x), ##__VA_ARGS__)
#define LOG(x, ...)

#define KEY_INVALID 127
#define KEY_EXTRA   -32        /*sent before certain keys such as up, down, left, or right( */

enum KeyMapSpecial
{
    SPECIAL_SHIFT = 1,
    SPECIAL_ALT,
    SPECIAL_CTRL,
    SPECIAL_SHIFTLOCK,
    SPECIAL_FONCTION,
    SPECIAL_WIN
};

struct keymap
{
    char normal;
    char shifted;
    char ctrled;
    char special;
};
static struct keymap keymap[] = {
#include <keymap.us.txt>
};

static int shift_mode = 0;
static int alt_mode = 0;
static int ctrl_mode = 0;
static int shiftlock_mode = 0;
static int win_mode = 0;

static struct Console *consoleLists[12] = {0};
static int activeConsoleId = -1;

static struct Console *createConsole();

void initConsole()
{
    consoleSwitchById(0);
}

static struct Console *createConsole()
{
    struct Console *newConsole = kmalloc(sizeof(struct Console), 0, "newConsole");
    if (!newConsole)
        return NULL;

    newConsole->readBuffer.readPtr = 0;
    newConsole->readBuffer.writePtr = 0;

    newConsole->mode = ConsoleModeText;
    newConsole->videoBuffer = kmalloc(VGA_VIDEO_WIDTH * VGA_VIDEO_HEIGHT, 0, "videobuffer");
    memset(newConsole->videoBuffer, 0, VGA_VIDEO_WIDTH * VGA_VIDEO_HEIGHT);

    newConsole->tty = createTerminal();

    mutexReset(&newConsole->mtx);
    newConsole->readingTask = NULL;
    newConsole->activeProcess = NULL;

    return newConsole;
}

int consoleSwitchById(int id)
{
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

struct Console *consoleGetActiveConsole()
{
    return consoleLists[activeConsoleId];
}

char consoleGetLastkey(struct Console *console)
{
    struct CirBuffer *val = &console->readBuffer;

    if (val->readPtr == val->writePtr)
        return -1;

    if (val->writePtr == 0)
        val->writePtr = CONSOLE_BUFFER_SIZE - 1;
    else
        val->writePtr -= 1;
    return val->buffer[val->writePtr];
}

static char keyboard_map(struct Console *cons, int code)
{
    int direction;

    if (code & 0x80) {
        direction = 0;
        code = code & 0x7f;
    } else {
        direction = 1;
    }

    if (keymap[code].special == SPECIAL_SHIFT) {
        shift_mode = direction;
        return KEY_INVALID;
    } else if (keymap[code].special == SPECIAL_ALT) {
        alt_mode = direction;
        return KEY_INVALID;
    } else if (keymap[code].special == SPECIAL_CTRL) {
        ctrl_mode = direction;
        return KEY_INVALID;
    } else if (keymap[code].special == SPECIAL_WIN) {
        win_mode = direction;
        return KEY_INVALID;
    } else if (keymap[code].special == SPECIAL_SHIFTLOCK) {
        if (direction == 0)
            shiftlock_mode = !shiftlock_mode;
        return KEY_INVALID;
    } else if (direction) {
        if (keymap[code].special == SPECIAL_FONCTION) {
            int consId = code - 59;
            char isNotReady = (consoleLists[consId] == NULL);
            consoleSwitchById(consId);
            if (isNotReady)
                createNewTTY();
            return KEY_INVALID;
        } else if ((ctrl_mode && keymap[code].normal == 'c') || (win_mode && keymap[code].normal == 'q')) {
            taskKill(cons->activeProcess);
            return KEY_INVALID;
        } else if (shiftlock_mode) {
            if (shift_mode) {
                return keymap[code].normal;
            } else {
                return keymap[code].shifted;
            }
        } else if (shift_mode) {
            return keymap[code].shifted;
        } else if (ctrl_mode) {
            return keymap[code].ctrled;
        } else {
            return keymap[code].normal;
        }
    } else {
        return KEY_INVALID;
    }
}

void consoleKeyboardHandler(int code)
{
    static char mod = 0x00;

    LOG("keyboard: %d\n", code);
    struct Console *cons = consoleLists[activeConsoleId];


    char c;
    if (code == KEY_EXTRA) {
        mod = 0x80;
        return;
    } else {
        c = keyboard_map(cons, code) | mod;
        mod = 0x00;
    }

    if (c == KEY_INVALID)
        return;


    if (c == ASCII_BS) {
        c = consoleGetLastkey(cons);
        if (c != -1)
            terminalRemoveLastChar(cons->tty, cons->mode == ConsoleModeText);

        return;
    }

    struct CirBuffer *val = &cons->readBuffer;

    if ((val->writePtr + 1) % CONSOLE_BUFFER_SIZE == val->readPtr)
        val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;

    val->buffer[val->writePtr] = c;
    val->tmpBuffer[val->writePtr] = code;
    val->writePtr = (val->writePtr + 1) % CONSOLE_BUFFER_SIZE;

    if (cons->mode == ConsoleModeText) {
        terminalPutchar(cons->tty, 1, c);
        terminalUpdateCursor(cons->tty);
    }

    if (c == ASCII_LF)
        taskResetEvent(cons->readingTask);
}

char consoleGetkey(struct Console *console)
{
    struct CirBuffer *val = &console->readBuffer;

    if (val->readPtr == val->writePtr)
        return -1;

    char tmp = val->buffer[val->readPtr];
    val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;
    return tmp;
}

int consoleGetkey2(struct Console *console)
{
    struct CirBuffer *val = &console->readBuffer;

    if (val->readPtr == val->writePtr)
        return 0;

    int tmp = val->tmpBuffer[val->readPtr];
    val->readPtr = (val->readPtr + 1) % CONSOLE_BUFFER_SIZE;
    return tmp;
}

static u32 consoleInternalRead(struct Console *console, char *str, u32 size)
{
    u32 read = 0;
    char key;

    while (read < size && (key = consoleGetkey(console)) != -1) {
        str[read] = key;
        read++;
    }

    console->readingTask = NULL;
    return read;
}

static char isConsoleReadReady(struct Console *console)
{
    struct CirBuffer *val = &console->readBuffer;

    int tmp = val->readPtr;
    while (tmp != val->writePtr) {
        if (val->buffer[tmp] == '\n')
            return 1;
        tmp = (tmp + 1) % CONSOLE_BUFFER_SIZE;
    }
    return 0;
}

s32 consoleReadKeyboard(void *entryData, void *buf, u32 size)
{
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

    return ret;
}

s32 consoleWriteStandard(void *entryData, const char *buf, u32 size)
{
    struct Console *cons = entryData;
    char writing = (cons->mode == ConsoleModeText && consoleLists[activeConsoleId] == cons);

    for (u32 i = 0; i < size; i++) {
        terminalPutchar(cons->tty, writing, buf[i]);
    }

    if (writing)
        terminalUpdateCursor(cons->tty);
    return size;
}

s32 consoleForceWrite(const char *buf, u32 size)
{
    return consoleWriteStandard(consoleLists[activeConsoleId], buf, size);
}

int consoleSwitchVideoMode(struct Console *console, enum ConsoleMode mode)
{
    if (console->mode == mode)
        return 0;

    console->mode = mode;

    if (console == consoleLists[activeConsoleId]) {
        switchVgaMode(mode);

        if (mode == ConsoleModeText)
            updateTerminal(console->tty);
        else
            setVgaFrameBuffer(console->videoBuffer);
    }
    return 0;
}

int consoleSetVgaFrameBuffer(struct Console *console, const void *buffer)
{
    if (console->mode != ConsoleModeVideo)
        return -EPERM;

    memcpy(console->videoBuffer, buffer, VGA_VIDEO_HEIGHT * VGA_VIDEO_WIDTH);
    if (console == consoleLists[activeConsoleId])
        setVgaFrameBuffer(buffer);
    return 0;
}