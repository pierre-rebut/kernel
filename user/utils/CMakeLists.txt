add_executable(mount mount.c)
target_link_libraries(mount stdc)

add_executable(umount umount.c)
target_link_libraries(umount stdc)

add_executable(cat cat2.c)
target_link_libraries(cat stdc)

add_executable(touch touch.c)
target_link_libraries(touch stdc)

add_executable(wc wc.c)
target_link_libraries(wc stdc)

add_executable(link link.c)
target_link_libraries(link stdc)

add_executable(mkdir mkdir.c)
target_link_libraries(mkdir stdc)

add_executable(chmod chmod.c)
target_link_libraries(chmod stdc)

add_executable(rm rm.c)
target_link_libraries(rm stdc)

add_executable(test_fork test.c)
target_link_libraries(test_fork stdc)