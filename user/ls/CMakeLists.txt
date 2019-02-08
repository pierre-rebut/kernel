set(SOURCE
        my_ls.c
        my_ls_fct.c
        my_ls_fct2.c
        my_ls_utils.c
        my_ls_utils2.c)

add_executable(ls ${SOURCE})
target_link_libraries(ls k c bin-entry)
