set(SOURCE yakanoid.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/yakanoid
        ball.bmp
        bar.bmp
        blue.bmp
        green.bmp
        red.bmp
        yaka.bmp)

add_executable(yakanoid ${SOURCE})
target_link_libraries(yakanoid k c bin-entry)

add_custom_command(TARGET yakanoid
        POST_BUILD
        DEPENDS mkkfs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating yakanoid rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o ${CMAKE_CURRENT_BINARY_DIR}/yakanoid.rom -n yakanoid ${FILES})


