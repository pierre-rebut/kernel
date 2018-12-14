#ifndef KERNEL_EPITA_TERMINAL_H
#define KERNEL_EPITA_TERMINAL_H

#include <k/types.h>
#include <k/kstd.h>

void initTerminal();
void clearTerminal();
void updateTerminalCursor();
void setTerminalX(u16 x);
void setTerminalY(u16 y);
void setTerminalColor(enum e_cons_codes fg, enum e_cons_codes bg);
void writeTerminal(char c);
void writeTerminalAt(char c, u8 color, u16 x, u16 y);
int writeStringTerminal(const char *data, u32 size);

int writeTerminalFromFD(void *tmp, void *data, u32 size);

#endif //KERNEL_EPITA_TERMINAL_H
