include_directories(${CMAKE_SOURCE_DIR}/libs/libc/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/libk/include)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/roms.lds")

add_subdirectory(42sh)
add_subdirectory(roms)
add_subdirectory(ls)
add_subdirectory(ls2)
add_subdirectory(nano)
add_subdirectory(cp)
add_subdirectory(utils)