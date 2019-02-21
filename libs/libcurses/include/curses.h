//
// Created by rebut_p on 21/02/19.
//

#ifndef KERNEL_CURSES_H
#define KERNEL_CURSES_H

#include <stdarg.h>
#include <stddef.h>
#include "key-curses.h"

typedef u32 chtype;

typedef struct {

} WINDOW;

WINDOW *initscr(void);
WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x);
int delwin(WINDOW *win);
int endwin(void);
bool isendwin(void);
int wnoutrefresh(WINDOW *win);
int doupdate(void);
int refresh(void);
int wrefresh(WINDOW *win);

int curs_set(int visibility);
int napms(int ms);

int wgetch(WINDOW *win);
int ungetch(int ch);

int printw(const char *fmt, ...);
int wprintw(WINDOW *win, const char *fmt, ...);
int mvprintw(int y, int x, const char *fmt, ...);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
int vwprintw(WINDOW *win, const char *fmt, va_list varglist);
int vw_printw(WINDOW *win, const char *fmt, va_list varglist);

int keypad(WINDOW *win, bool bf);
int echo(void);
int noecho(void);
int raw(void);
int noraw(void);
int nl(void);
int nonl(void);
int nodelay(WINDOW *win, bool bf);

int wattron(WINDOW *win, int attrs);
int waddstr(WINDOW *win, const char *str);
int waddnstr(WINDOW *win, const char *str, int n);
int mvwaddstr(WINDOW *win, int y, int x, const char *str);
int mvwaddnstr(WINDOW *win, int y, int x, const char *str, int n);
int waddch(WINDOW *win, const chtype ch);
int mvwaddch(WINDOW *win, int y, int x, const chtype ch);
int wattroff(WINDOW *win, int attrs);

int move(int y, int x);
int wmove(WINDOW *win, int y, int x);

int scrollok(WINDOW *win, bool bf);
int wscrl(WINDOW *win, int n);

extern chtype acs_map[];
extern WINDOW *curscr;
extern WINDOW *newscr;
extern WINDOW *stdscr;
extern char ttytype[];
extern int COLORS;
extern int COLOR_PAIRS;
extern int COLS;
extern int ESCDELAY;
extern int LINES;
extern int TABSIZE;
//extern cchar_t *_nc_wacs;
extern int _nc_optimize_enable;

#define ERR (-1)

#define NCURSES_ATTR_SHIFT       8
#define NCURSES_CAST(type,value) (type)(value)
#define NCURSES_BITS(mask,shift) (NCURSES_CAST(chtype,(mask)) << ((shift) + NCURSES_ATTR_SHIFT))

#define A_NORMAL	(1U - 1U)

#define A_ATTRIBUTES	NCURSES_BITS(~(1U - 1U),0)
#define A_CHARTEXT	(NCURSES_BITS(1U,0) - 1U)
#define A_COLOR		NCURSES_BITS(((1U) << 8) - 1U,0)
#define A_STANDOUT	NCURSES_BITS(1U,8)
#define A_UNDERLINE	NCURSES_BITS(1U,9)
#define A_REVERSE	NCURSES_BITS(1U,10)
#define A_BLINK		NCURSES_BITS(1U,11)
#define A_DIM		NCURSES_BITS(1U,12)
#define A_BOLD		NCURSES_BITS(1U,13)
#define A_ALTCHARSET	NCURSES_BITS(1U,14)
#define A_INVIS		NCURSES_BITS(1U,15)
#define A_PROTECT	NCURSES_BITS(1U,16)
#define A_HORIZONTAL	NCURSES_BITS(1U,17)
#define A_LEFT		NCURSES_BITS(1U,18)
#define A_LOW		NCURSES_BITS(1U,19)
#define A_RIGHT		NCURSES_BITS(1U,20)
#define A_TOP		NCURSES_BITS(1U,21)
#define A_VERTICAL	NCURSES_BITS(1U,22)
#define A_ITALIC	NCURSES_BITS(1U,23)	/* ncurses extension */
#define COLOR_PAIR(n)	(NCURSES_BITS((n), 0) & A_COLOR)


#endif //KERNEL_CURSES_H
