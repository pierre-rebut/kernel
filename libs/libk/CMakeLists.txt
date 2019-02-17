include_directories(${CMAKE_SOURCE_DIR}/libs/include)
include_directories(include)

set(SOURCE
        calloc.c
        graphic.c
        malloc.c
        realloc.c
        sound.c
        strdup.c
        unistd.c
        puts.c
        getopt.c include/getopt.h
        include/sound.h include/graphic.h include/assert.h include/err.h include/unistd.h
        filestream.c include/filestream.h
        fts.c include/fts.h
        errno.c include/errno.h dirent.c include/dirent.h syscalls.h syscalls.c
        getenv.c stdio.c include/getnbr.h time.c include/time.h scanf.c filestream-input.c utils.c include/utils.h include/scanf.h didiv.c)

add_library(stdk STATIC ${SOURCE})