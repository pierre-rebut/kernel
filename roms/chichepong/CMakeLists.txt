set(SOURCE pong.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pong
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.ksf
        ${CMAKE_CURRENT_SOURCE_DIR}/chiche.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/intro.ksf)

add_executable(pong ${SOURCE} ${ENTRYFILE})
target_link_libraries(pong k c)

add_custom_command(TARGET pong
        POST_BUILD
        DEPENDS mkkfs
        COMMENT "Creating pong rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o pong.rom -n pong ${FILES})

