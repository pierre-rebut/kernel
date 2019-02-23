set(SOURCE
        ls.c cmp.c extern.h print.c utils.c ls.h)

add_executable(ls ${SOURCE})
target_link_libraries(ls stdc)
