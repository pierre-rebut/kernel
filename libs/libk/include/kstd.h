#ifndef KSTD_H
#define KSTD_H

#include "types.h"

typedef s32 ssize_t;
typedef s32 off_t;

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

/* misc */
#define O_RDONLY	0
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#define VIDEO_GRAPHIC	0
#define VIDEO_TEXT	1

/*
** syscalls
*/

#define SYSCALL_EXIT            0
#define SYSCALL_SBRK			1
#define SYSCALL_GETKEY			2
#define SYSCALL_GETTICK			3
#define SYSCALL_OPEN			4
#define SYSCALL_READ			5
#define SYSCALL_WRITE			6
#define SYSCALL_SEEK			7
#define SYSCALL_CLOSE			8
#define SYSCALL_SETVIDEO		9
#define SYSCALL_SWAP_FRONTBUFFER	10
#define SYSCALL_PLAYSOUND		11
#define SYSCALL_GETMOUSE		12 /* XXX: not implemented */
#define SYSCALL_USLEEP		    13
#define SYSCALL_WAITPID	    	14
#define SYSCALL_KILL		    15
#define SYSCALL_GETPID  		16
#define SYSCALL_EXECVE  		17
#define SYSCALL_STAT     		18
#define SYSCALL_FSTAT     		19
#define SYSCALL_CHDIR           20
#define SYSCALL_GETKEYMODE		21
#define SYSCALL_OPENDIR         22
#define SYSCALL_CLOSEDIR        23
#define SYSCALL_READDIR         24

int exit(int value);
void *sbrk(ssize_t increment);
int getkey(void);
unsigned long gettick(void);
int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t count);
int write(int fd, const void *s, size_t length);
off_t seek(int filedes, off_t offset, int whence);
int close(int fd);
int setvideo(int mode);
void swap_frontbuffer(const void *buffer);
int playsound(struct melody *melody, int repeat);
int getmouse(int *x, int *y, int *buttons);
int getkeymode(int mode);
int uslepp(u32 duration);
u32 kill(u32 pid);
int waitpid(u32 pid);
u32 getpid();
int sleep(u32 duration);
u32 execve(const char *prg, const char **av, const char **env);
int stat(const char *pathname, struct stat *data);
int fstat(int fd, struct stat *data);
int chdir(const char *path);
int opendir(const char *name);
int closedir(int repertoire);
struct dirent* readdir(int repertoire);

#endif
