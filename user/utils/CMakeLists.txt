add_executable(mount mount.c)
target_link_libraries(mount stdk stdc bin-entry)

add_executable(umount umount.c)
target_link_libraries(umount stdk stdc bin-entry)

add_executable(cat cat.c)
target_link_libraries(cat stdk stdc bin-entry)