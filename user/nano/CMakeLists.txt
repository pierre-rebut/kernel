include_directories(${CMAKE_SOURCE_DIR}/libs/libcurses/include)

set(SOURCE
        browser.c
        chars.c
        color.c
        cut.c
        files.c
        global.c
        help.c
        history.c
        move.c
        nano.c nano.h
        prompt.c
        proto.h
        rcfile.c
        revision.h
        search.c
        text.c
        utils.c
        winio.c)

add_executable(nano ${SOURCE})
target_link_libraries(nano stdk stdc crt0 stdcurses)
