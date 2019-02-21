#ifndef KSTD_H
#define KSTD_H

#include "types.h"

struct melody
{
    unsigned long freq;
    unsigned long duration;
};

/*
** constants
*/

/* console */
enum e_cons_codes
{
    CONS_ESCAPE = 255,
    CONS_CLEAR = 1,
    CONS_COLOR = 2,
    CONS_SETX = 3,
    CONS_SETY = 4,
    CONS_BLACK = 0,
    CONS_BLUE = 1,
    CONS_GREEN = 2,
    CONS_CYAN = 3,
    CONS_RED = 4,
    CONS_MAGENTA = 5,
    CONS_YELLOW = 6,
    CONS_WHITE = 7,
    CONS_BLINK = (1 << 7),
    CONS_LIGHT = (1 << 3)
};

#define CONS_FRONT(Color)    (Color)
#define CONS_BACK(Color)    (Color << 4)

/* keyboard */
enum e_kbd_codes
{
    K_KEY_ESC = 1,
    K_KEY_F1 = 59,
    K_KEY_F2 = 60,
    K_KEY_F3 = 61,
    K_KEY_F4 = 62,
    K_KEY_F5 = 63,
    K_KEY_F6 = 64,
    K_KEY_F7 = 65,
    K_KEY_F8 = 66,
    K_KEY_F9 = 67,
    K_KEY_F10 = 68,
    K_KEY_F11 = 87,
    K_KEY_F12 = 88,
    K_KEY_1 = 2,
    K_KEY_2 = 3,
    K_KEY_3 = 4,
    K_KEY_4 = 5,
    K_KEY_5 = 6,
    K_KEY_6 = 7,
    K_KEY_7 = 8,
    K_KEY_8 = 9,
    K_KEY_9 = 10,
    K_KEY_0 = 11,
    K_KEY_TAB = 15,
    K_KEY_MAJLOCK = 58,
    K_KEY_LSHIFT = 42,
    K_KEY_RSHIFT = 54,
    K_KEY_ALT = 56,
    K_KEY_SPACE = 57,
    K_KEY_CTRL = 29,
    K_KEY_ENTER = 28,
    K_KEY_BACKSPACE = 14,
    K_KEY_LEFT = 75,
    K_KEY_RIGHT = 77,
    K_KEY_UP = 72,
    K_KEY_DOWN = 80,
    K_KEY_PAUSE = 69,
    K_KEY_SYST = 55,
    K_KEY_INSER = 82,
    K_KEY_SUPPR = 83,
};

enum e_k_mode
{
    KEY_PRESSED,
    KEY_RELEASED,
};

/* mouse */
enum e_mouse_codes
{
    BUTTON_LEFT = 1,
    BUTTON_RIGHT = 2
};

enum SysConfigInfo
{
    _SC_PHYS_PAGES,
    _SC_PAGESIZE
};

/* misc */
#define O_ACCMODE       0003
#define O_RDONLY         00
#define O_WRONLY         01
#define O_RDWR             02
#define O_CREAT        0100
#define O_EXCL           0200
#define O_TRUNC          01000
#define O_APPEND      02000

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#define VIDEO_GRAPHIC    0
#define VIDEO_TEXT    1

#define MAXPATHLEN 255

#endif
