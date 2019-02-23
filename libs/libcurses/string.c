//
// Created by rebut_p on 22/02/19.
//

#include <curses.h>

int wattron(WINDOW *win, int attrs)
{
    (void) win;
    (void) attrs;
    return 0;
}

int waddstr(WINDOW *win, const char *str)
{
    (void) win;
    (void) str;
    return 0;
}

int waddnstr(WINDOW *win, const char *str, int n)
{
    (void) win;
    (void) str;
    (void) n;
    return 0;
}

int mvwaddstr(WINDOW *win, int y, int x, const char *str)
{
    (void) win;
    (void)y;
    (void)x;
    (void)str;
    return 0;
}

int mvwaddnstr(WINDOW *win, int y, int x, const char *str, int n)
{
    (void) win;
    (void)y;
    (void)x;
    (void)str;
    (void)n;
    return 0;
}

int waddch(WINDOW *win, const chtype ch)
{
    (void) win;
    (void)ch;
    return 0;
}

int mvwaddch(WINDOW *win, int y, int x, const chtype ch)
{
    (void) win;
    (void)y;
    (void)x;
    (void)ch;
    return 0;
}

int wattroff(WINDOW *win, int attrs)
{
    (void) win;
    (void) attrs;
    return 0;
}