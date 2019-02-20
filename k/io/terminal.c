#include <sys/allocator.h>
#include <include/ascii.h>
#include "terminal.h"
#include "io.h"

#define VGA_VIDEO_WIDTH 80
#define VGA_VIDEO_HEIGHT 24
#define VGA_MEMORY 0xB8000

static u16 *terminalBuffer = (void *) VGA_MEMORY;

struct TerminalBuffer *createTerminal()
{
    struct TerminalBuffer *tty = kmalloc(sizeof(struct TerminalBuffer), 0, "tty");
    if (tty == NULL)
        return NULL;

    tty->terminalData = kmalloc(sizeof(u16) * VGA_VIDEO_HEIGHT * VGA_VIDEO_WIDTH, 0, "ttydata");
    setTerminalColor(tty, CONS_WHITE, CONS_BLACK);
    clearTerminal(tty);

    return tty;
}

void updateTerminal(struct TerminalBuffer *tty)
{
    for (u32 y = 0; y < VGA_VIDEO_HEIGHT; y++)
        for (u32 x = 0; x < VGA_VIDEO_WIDTH; x++)
            terminalBuffer[y * VGA_VIDEO_WIDTH + x] = tty->terminalData[y * VGA_VIDEO_WIDTH + x];

    terminalUpdateCursor(tty);
}

static inline u16 vgaEntry(char uc, u8 color)
{
    return (u16) uc | (u16) color << 8;
}

static void scroll(struct TerminalBuffer *tty, char writing)
{
    u16 *buf = tty->terminalData;

    for (u32 y = 1; y < VGA_VIDEO_HEIGHT; y++) {
        for (u32 x = 0; x < VGA_VIDEO_WIDTH; x++) {
            u32 index = (y - 1) * VGA_VIDEO_WIDTH + x;
            buf[index] = buf[y * VGA_VIDEO_WIDTH + x];

            if (writing)
                terminalBuffer[index] = buf[index];
        }
    }

    for (u32 x = 0; x < VGA_VIDEO_WIDTH; x++) {
        u32 index = (VGA_VIDEO_HEIGHT - 1) * VGA_VIDEO_WIDTH + x;
        buf[index] = vgaEntry(' ', tty->terminalColor);

        if (writing)
            terminalBuffer[index] = buf[index];
    }
}

static void newline(struct TerminalBuffer *tty, char writing)
{
    tty->terminalCol = 0;
    if (tty->terminalRow + 1 == VGA_VIDEO_HEIGHT)
        scroll(tty, writing);
    else
        tty->terminalRow++;
}

void terminalUpdateCursor(struct TerminalBuffer *tty)
{
    u16 offset = (tty->terminalRow * (u16) VGA_VIDEO_WIDTH) + tty->terminalCol;
    outb(0x3D4, 14);
    outb(0x3D5, (u8) (offset >> 8));
    outb(0x3D4, 15);
    outb(0x3D5, (u8) offset);
}

void clearTerminal(struct TerminalBuffer *tty)
{
    for (u32 i = 0; i < VGA_VIDEO_HEIGHT * VGA_VIDEO_WIDTH; i++) {
        terminalBuffer[i] = tty->terminalData[i] = vgaEntry(' ', tty->terminalColor);
    }

    tty->terminalCol = 0;
    tty->terminalRow = 0;
    terminalUpdateCursor(tty);
}

void setTerminalColor(struct TerminalBuffer *tty, enum e_cons_codes fg, enum e_cons_codes bg)
{
    tty->terminalColor = fg | bg << 4;
}

void terminalPutchar(struct TerminalBuffer *tty, char writing, char c)
{
    switch (c) {
        case ASCII_LF:
            newline(tty, writing);
            break;

        case ASCII_HT:
            tty->terminalCol += 4;
            break;

        case ASCII_SP:
            tty->terminalCol++;
            break;

        default: {
            const u16 index = tty->terminalRow * (u16) VGA_VIDEO_WIDTH + tty->terminalCol;
            tty->terminalData[index] = vgaEntry(c, tty->terminalColor);
            tty->terminalCol++;

            if (writing)
                terminalBuffer[index] = tty->terminalData[index];
        }
    }

    if (tty->terminalCol >= VGA_VIDEO_WIDTH)
        newline(tty, writing);
}

void terminalRemoveLastChar(struct TerminalBuffer *tty, char writing)
{
    if (tty->terminalCol == 0) {
        tty->terminalCol = VGA_VIDEO_WIDTH;

        if (tty->terminalRow == 0)
            tty->terminalRow = VGA_VIDEO_HEIGHT;
        tty->terminalRow -= 1;
    }

    tty->terminalCol -= 1;
    u16 index = tty->terminalRow * (u16) VGA_VIDEO_WIDTH + tty->terminalCol;
    tty->terminalData[index] = vgaEntry(' ', tty->terminalColor);

    if (writing) {
        terminalBuffer[index] = tty->terminalData[index];
        terminalUpdateCursor(tty);
    }
}