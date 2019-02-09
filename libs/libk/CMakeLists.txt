include_directories(../include)

set(SOURCE
        calloc.c
        graphic.c
        malloc.c
        realloc.c
        sound.c
        strdup.c
        syscalls.c)

add_library(k STATIC ${SOURCE})