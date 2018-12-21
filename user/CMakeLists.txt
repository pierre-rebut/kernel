include_directories(${CMAKE_SOURCE_DIR}/libs/libc/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/libk/include)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/roms.lds")

set_property(SOURCE bin-entry.S PROPERTY LANGUAGE C)
add_library(bin-entry bin-entry.S)

add_subdirectory(roms)
add_subdirectory(minishell)
add_subdirectory(ls)
add_subdirectory(mount)