//
// Created by rebut_p on 21/02/19.
//

#include <curses.h>

int printw(const char *fmt, ...)
{
    // todo
    (void) fmt;
    return 0;
}

int wprintw(WINDOW *win, const char *fmt, ...)
{
    // todo
    (void)win;
    (void)fmt;
    return 0;
}

int mvprintw(int y, int x, const char *fmt, ...)
{
    // todo
    (void)x;
    (void)y;
    (void)fmt;
    return 0;
}

int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
    // todo
    (void)win;
    (void)x;
    (void)y;
    (void)fmt;
    return 0;
}

int vwprintw(WINDOW *win, const char *fmt, va_list varglist)
{
    // todo
    (void)win;
    (void)fmt;
    (void)varglist;
    return 0;
}

int vw_printw(WINDOW *win, const char *fmt, va_list varglist)
{
    // todo
    (void)win;
    (void)fmt;
    (void)varglist;
    return 0;
}