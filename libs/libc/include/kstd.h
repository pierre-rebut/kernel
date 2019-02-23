//
// Created by rebut_p on 16/02/19.
//

#ifndef _KSTD_H
#define _KSTD_H

#include "ctype.h"

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

#endif // _KSTD_H
