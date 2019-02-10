include_directories(${CMAKE_SOURCE_DIR}/libs/include)

set(SOURCE
        calloc.c
        graphic.c
        malloc.c
        realloc.c
        sound.c
        strdup.c
        syscalls.c
        puts.c)

add_library(stdk STATIC ${SOURCE})