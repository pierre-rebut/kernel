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
        include/sound.h include/graphic.h include/assert.h include/err.h include/unistd.h
        filestream.c include/filestream.h
        fts.c include/fts.h
        errno.c include/errno.h dirent.c include/dirent.h syscalls.h)

add_library(stdk STATIC ${SOURCE})