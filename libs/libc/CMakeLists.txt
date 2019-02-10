include_directories(${CMAKE_SOURCE_DIR}/libs/include)

set(LIBC_SOURCE
        mem.c
        memmove.c
        stdlib.c
        string.c
        stringn.c
        printf.c)

add_library(stdc STATIC ${LIBC_SOURCE})