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

add_executable(mysh ${SOURCE})
target_link_libraries(mysh k c bin-entry)
