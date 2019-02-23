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