include_directories(../include)

set(LIBC_SOURCE
        mem.c
        memmove.c
        stdlib.c
        string.c
        stringn.c
        printf.c)

add_library(c STATIC ${LIBC_SOURCE})