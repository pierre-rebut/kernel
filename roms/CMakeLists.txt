include_directories(${CMAKE_SOURCE_DIR}/libs/libc/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/libk/include)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/roms.lds")

add_subdirectory(chichehunter)
add_subdirectory(chichepong)
add_subdirectory(chichevaders)
add_subdirectory(perrodlauncher)
add_subdirectory(skate)
add_subdirectory(yakanoid)

