set(SOURCE hunter.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/hunter
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.csf
        ${CMAKE_CURRENT_SOURCE_DIR}/bush.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/chef.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/chef_big.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/chiche.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/chiche_big.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/intro.csf
        ${CMAKE_CURRENT_SOURCE_DIR}/seine_w.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/test)

add_executable(hunter ${SOURCE})
target_link_libraries(hunter k c bin-entry)

add_custom_command(TARGET hunter
        POST_BUILD
        DEPENDS mkkfs
        COMMENT "Creating hunter rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o hunter.rom -n hunter ${FILES})
