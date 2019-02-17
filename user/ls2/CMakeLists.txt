set(SOURCE
        ls.c cmp.c extern.h print.c utils.c ls.h)

add_executable(ls2 ${SOURCE})
target_link_libraries(ls2 stdk stdc crt0)
