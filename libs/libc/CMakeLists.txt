include_directories(${CMAKE_SOURCE_DIR}/libs/include)

set(LIBC_SOURCE
        mem.c
        memmove.c
        stdlib.c
        string.c
        stringn.c
        printf.c getopt.c
        ../include/getopt.h ../include/kstd.h ../include/string.h
        ../include/stdio.h ../include/stdlib.h
        ../include/types.h
        qsort.c ../include/errno-base.h)

add_library(stdc STATIC ${LIBC_SOURCE})