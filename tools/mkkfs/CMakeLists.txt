set(CMAKE_C_FLAGS "-Wall -Wextra -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "")

set(SOURCE mkkfs.c)

add_executable(mkkfs ${SOURCE})