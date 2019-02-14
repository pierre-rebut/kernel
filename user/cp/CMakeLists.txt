set(SOURCE
        cp.c
        utils.c
        extern.h)

add_executable(cp ${SOURCE})
target_link_libraries(cp stdk stdc crt0)
