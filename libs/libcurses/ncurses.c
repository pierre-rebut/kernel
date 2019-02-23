//
// Created by rebut_p on 21/02/19.
//

#include <stddef.h>
#include <curses.h>

chtype acs_map[];
WINDOW *curscr = NULL;
WINDOW *newscr = NULL;
WINDOW *stdscr = NULL;
char ttytype[];
int COLORS;
int COLOR_PAIRS;
int COLS;
int ESCDELAY;
int LINES;
int TABSIZE;
//cchar_t *_nc_wacs;
int _nc_optimize_enable;

WINDOW *initscr(void)
{
    // todo
    return NULL;
}

int endwin(void)
{
    // todo
    return 0;
}

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
    (void) nlines;
    (void) ncols;
    (void) begin_x;
    (void) begin_y;
    return NULL;
}

int delwin(WINDOW *win)
{
    (void) win;
    return 0;
}

bool isendwin(void)
{
    return false;
}

int wgetch(WINDOW *win)
{
    // todo
    (void) win;
    return 0;
}

int ungetch(int ch)
{
    (void) ch;
    return 0;
}

int wnoutrefresh(WINDOW *win)
{
    (void) win;
    return 0;
}

int doupdate(void)
{
    return 0;
}

int refresh(void)
{
    return 0;
}

int wrefresh(WINDOW *win)
{
    (void) win;
    return 0;
}

int curs_set(int visibility)
{
    (void) visibility;
    return 0;
}

int napms(int ms)
{
    (void) ms;
    return 0;
}