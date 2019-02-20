#ifndef KERNEL_EPITA_TERMINAL_H
#define KERNEL_EPITA_TERMINAL_H

#include <k/ktypes.h>
#include <kstd.h>

struct TerminalBuffer
{
    u16 *terminalData;
    u16 terminalRow;
    u16 terminalCol;
    u8 terminalColor;
};

struct TerminalBuffer *createTerminal();

void clearTerminal(struct TerminalBuffer *tty);

void updateTerminal(struct TerminalBuffer *tty);

void setTerminalColor(struct TerminalBuffer *tty, enum e_cons_codes fg, enum e_cons_codes bg);

void terminalUpdateCursor(struct TerminalBuffer *tty);

void terminalPutchar(struct TerminalBuffer *tty, char writing, char c);

void terminalRemoveLastChar(struct TerminalBuffer *tty, char writing);

#endif //KERNEL_EPITA_TERMINAL_H
