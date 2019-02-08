set(SOURCE pong.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pong
        ball.bmp
        ball.ksf
        chiche.bmp
        intro.ksf)

add_executable(pong ${SOURCE})
target_link_libraries(pong k c bin-entry)

add_custom_command(TARGET pong
        POST_BUILD
        DEPENDS mkkfs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating pong rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o ${CMAKE_CURRENT_BINARY_DIR}/pong.rom -n pong ${FILES})

