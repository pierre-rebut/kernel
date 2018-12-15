include_directories(include)

set(LIBC_SOURCE
        memset.c
        memcpy.c
        memcmp.c
        printf.c
        puts.c
        strnlen.c
        strncmp.c
        strcpy.c
        strcmp.c
        strlen.c
        strncpy.c strcat.c)

add_library(c STATIC ${LIBC_SOURCE})