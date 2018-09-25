#include "terminal.h"
#include "io.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 24
#define VGA_MEMORY 0xB8000

static u16 terminalRow;
static u16 terminalColumn;

static u8 terminalColor;
static u16 *terminalBuffer;

static inline u16 vgaEntry(char uc, u8 color) {
    return (u16) uc | (u16) color << 8;
}

static void scroll() {
    u16 *buf = terminalBuffer;

    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++)
            buf[(y - 1) * VGA_WIDTH + x] = buf[y * VGA_WIDTH + x];
    }

    for (size_t x = 0; x < VGA_WIDTH; x++)
        buf[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vgaEntry(' ', terminalColor);
}

static void newline() {
    terminalColumn = 0;
    if (terminalRow + 1 == VGA_HEIGHT)
        scroll();
    else
        terminalRow++;
}

void updateTerminalCursor(){
    u16 offset = (terminalRow * (u16)VGA_WIDTH) + terminalColumn;
    outb(0x3D4, 14);
    outb(0x3D5, (u8)(offset >> 8));
    outb(0x3D4, 15);
    outb(0x3D5, (u8)offset);
}

static void putchar(char c) {
    switch (c) {
        case '\n':
            newline();
            break;

        case '\r':
            terminalColumn = 0;
            break;

        case '\t':
            terminalColumn += 8;
            break;

        case ' ':
            terminalColumn++;
            break;

        default:
            writeTerminalAt(c, terminalColor, terminalColumn, terminalRow);
            terminalColumn++;
            break;
    }

    if (terminalColumn >= VGA_WIDTH)
        newline();
}

void initTerminal() {
    terminalBuffer = (u16 *) VGA_MEMORY;

    setTerminalColor(CONS_WHITE, CONS_BLACK);
    clearTerminal();
}

void clearTerminal() {
    for (size_t i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++)
        terminalBuffer[i] = vgaEntry(' ', terminalColor);

    terminalColumn = 0;
    terminalRow = 0;
    updateTerminalCursor();
}

void setTerminalColor(enum e_cons_codes fg, enum e_cons_codes bg) {
    terminalColor = fg | bg << 4;
}

void setTerminalX(u16 x) {
    terminalColumn = x;
}

void setTerminalY(u16 y) {
    terminalRow = y;
}

void writeTerminal(char c) {
    putchar(c);
    updateTerminalCursor();

}

void writeTerminalAt(char c, u8 color, u16 x, u16 y) {
    const u16 index = y * (u16)VGA_WIDTH + x;
    terminalBuffer[index] = vgaEntry(c, color);
}

int writeStringTerminal(const char *data, size_t size) {
    size_t i;
    for (i = 0; i < size; i++)
        putchar(data[i]);
    updateTerminalCursor();
    return (int)i;
}
