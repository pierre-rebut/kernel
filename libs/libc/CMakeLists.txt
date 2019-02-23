include_directories(include)

set(LIBK_SOURCE
        stdk/mem.c
        stdk/memmove.c
        stdk/printf.c
        stdk/qsort.c
        stdk/stdlib.c
        stdk/string.c
        stdk/stringn.c
        include/kernel/ctype.h
        include/kernel/errno-base.h
        include/kernel/limits.h
        include/kernel/sound.h
        include/kernel/stdarg.h
        include/kernel/stddef.h
        include/kernel/stdio.h
        include/kernel/stdlib.h
        include/kernel/string.h)

set(LIBC_SOURCE
        stdc/alloc/realloc.c stdc/alloc/calloc.c stdc/alloc/free.c stdc/alloc/malloc.c stdc/alloc/malloc.h
        stdc/alloc/show_alloc_mem.c
        stdc/didiv.c stdc/dirent.c stdc/errno.c stdc/filestream.c stdc/filestream-input.c
        stdc/fts.c stdc/getenv.c stdc/getopt.c stdc/graphic.c stdc/puts.c stdc/regex.c stdc/scanf.c stdc/signal.c
        stdc/sound.c stdc/stdio.c stdc/strdup.c stdc/sys.c stdc/syscalls.c stdc/syscalls.h stdc/time.c
        stdc/unistd.c stdc/utils.c
        include/sys/mman.h include/sys/mount.h include/sys/signum.h include/sys/stat.h include/sys/types.h
        include/sys/uio.h
        include/assert.h include/ctype.h include/dirent.h include/err.h include/errno.h include/filestream.h
        include/fts.h include/getopt.h include/graphic.h include/kstd.h include/limits.h include/regex.h
        include/signal.h include/sound.h include/stdarg.h include/stddef.h include/stdio.h include/stdlib.h
        include/string.h include/time.h include/unistd.h include/utils.h)

set(LIBCRT_SOURCE
        crt/crt0.S
        crt/crti.c)

set_property(SOURCE crt/crt0.S PROPERTY LANGUAGE C)

add_library(stdk STATIC ${LIBK_SOURCE})

add_library(stdc STATIC ${LIBK_SOURCE} ${LIBC_SOURCE} ${LIBCRT_SOURCE})