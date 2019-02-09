set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${CMAKE_CURRENT_SOURCE_DIR}/roms.lds")

set_property(SOURCE bin-entry.S PROPERTY LANGUAGE C)
add_library(bin-entry bin-entry.S)

add_subdirectory(42sh)
#add_subdirectory(roms)
add_subdirectory(ls)
add_subdirectory(utils)