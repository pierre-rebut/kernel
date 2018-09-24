#ifndef KERNEL_EPITA_TERMINAL_H
#define KERNEL_EPITA_TERMINAL_H

#include <stddef.h>
#include <k/kstd.h>

void initTerminal();
void clearTerminal();
void setTerminalX(size_t x);
void setTerminalY(size_t y);
void setTerminalColor(enum e_cons_codes fg, enum e_cons_codes bg);
void writeTerminal(char c);
void writeTerminalAt(char c, u8 color, size_t x, size_t y);
int writeStringTerminal(const char *data, size_t size);

#endif //KERNEL_EPITA_TERMINAL_H
