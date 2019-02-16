#ifndef KSTD_H
#define KSTD_H

#include "types.h"

struct melody {
    unsigned long freq;
    unsigned long duration;
};

/*
** constants
*/

/* console */
enum e_cons_codes {
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

#define CONS_FRONT(Color)	(Color)
#define CONS_BACK(Color)	(Color << 4)

/* keyboard */
enum e_kbd_codes {
    KEY_ESC = 1,
    KEY_F1 = 59,
    KEY_F2 = 60,
    KEY_F3 = 61,
    KEY_F4 = 62,
    KEY_F5 = 63,
    KEY_F6 = 64,
    KEY_F7 = 65,
    KEY_F8 = 66,
    KEY_F9 = 67,
    KEY_F10 = 68,
    KEY_F11 = 87,
    KEY_F12 = 88,
    KEY_1 = 2,
    KEY_2 = 3,
    KEY_3 = 4,
    KEY_4 = 5,
    KEY_5 = 6,
    KEY_6 = 7,
    KEY_7 = 8,
    KEY_8 = 9,
    KEY_9 = 10,
    KEY_0 = 11,
    KEY_TAB = 15,
    KEY_MAJLOCK = 58,
    KEY_LSHIFT = 42,
    KEY_RSHIFT = 54,
    KEY_ALT = 56,
    KEY_SPACE = 57,
    KEY_CTRL = 29,
    KEY_ENTER = 28,
    KEY_BACKSPACE = 14,
    KEY_LEFT = 75,
    KEY_RIGHT = 77,
    KEY_UP = 72,
    KEY_DOWN = 80,
    KEY_PAUSE = 69,
    KEY_SYST = 55,
    KEY_INSER = 82,
    KEY_SUPPR = 83,
};

enum e_k_mode {
    KEY_PRESSED,
    KEY_RELEASED,
};

/* mouse */
enum e_mouse_codes {
    BUTTON_LEFT = 1,
    BUTTON_RIGHT = 2
};

enum SysConfigInfo {
    _SC_PHYS_PAGES,
    _SC_PAGESIZE
};

/* misc */
#define O_ACCMODE	   0003
#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02
#define O_CREAT        0100
#define O_EXCL		   0200
#define O_TRUNC	      01000
#define O_APPEND	  02000

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#define VIDEO_GRAPHIC	0
#define VIDEO_TEXT	1

#define MAXPATHLEN 255

#endif
