include_directories(include)

set(SOURCE
        src/main.c
        src/exec.c
        src/create_list.c
        src/exec_prg.c
        src/parser.c
        src/exec_built.c
        src/redir.c
        src/builtins.c
        src/env.c
        src/current_dir.c
        src/list_check.c
        src/tools/str_parser_line.c
        src/tools/tools.c
        src/tools/memory.c
        src/tools/str.c
        src/tools/my_epur_cmd.c
        src/tools/my_wordtab.c
        src/parsing/check_expr_cmplx.c
        src/parsing/check.c
        src/parsing/new_check.c
        src/parsing/tools.c)

add_executable(42sh ${SOURCE})
target_link_libraries(42sh stdk stdc crt0)
