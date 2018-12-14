set(CMAKE_C_FLAGS "-Wall -Wextra -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "")

set(SOURCE mkksf.c)

add_executable(mkksf ${SOURCE})