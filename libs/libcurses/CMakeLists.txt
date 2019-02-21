include_directories(${CMAKE_SOURCE_DIR}/libs/libc/include)
include_directories(include)

set(SOURCE
        ncurses.c
        include/curses.h
        printw.c
        utils.c include/key-curses.h string.c)

add_library(stdcurses STATIC ${SOURCE})