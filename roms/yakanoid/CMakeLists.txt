set(SOURCE yakanoid.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/yakanoid
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/bar.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/blue.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/green.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/red.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/yaka.bmp)

add_executable(yakanoid ${SOURCE})
target_link_libraries(yakanoid k c bin-entry)

add_custom_command(TARGET yakanoid
        POST_BUILD
        DEPENDS mkkfs
        COMMENT "Creating yakanoid rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o yakanoid.rom -n yakanoid ${FILES})


