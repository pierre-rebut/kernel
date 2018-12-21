add_executable(mount mount.c)
target_link_libraries(mount k c bin-entry)

add_executable(umount umount.c)
target_link_libraries(umount k c bin-entry)