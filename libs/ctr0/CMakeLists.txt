include_directories(${CMAKE_SOURCE_DIR}/libs/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/libk/include)

set(CRT0_SOURCE
        init.c
        crt0.S)

set_property(SOURCE crt0.S PROPERTY LANGUAGE C)

add_library(crt0 STATIC ${CRT0_SOURCE})
target_link_libraries(crt0 stdk)