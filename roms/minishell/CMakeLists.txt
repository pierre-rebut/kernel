include_directories(include)

set(SOURCE
        main.c
        mysh.c
        builtin_oth.c
        mysh_fct_utils.c
        my_builtin.c
        get_next_line.c
        my_getenv.c
        mysh_fct.c
        mysh_utils.c
        mysh_utils2.c)

set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mysh
        ${CMAKE_CURRENT_BINARY_DIR}/../chichehunter/hunter
        ${CMAKE_CURRENT_BINARY_DIR}/../ls/ls
        ${CMAKE_CURRENT_SOURCE_DIR}/mysh.c
        ${CMAKE_CURRENT_SOURCE_DIR}/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/get_next_line.c)

add_executable(mysh ${SOURCE})
target_link_libraries(mysh k c bin-entry)

add_custom_command(TARGET mysh
        POST_BUILD
        DEPENDS mkkfs hunter ls
        COMMENT "Creating mysh rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o mysh.rom -n mysh ${FILES})
