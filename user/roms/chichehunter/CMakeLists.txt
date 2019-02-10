set(SOURCE hunter.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/hunter
        ball.bmp
        ball.csf
        bush.bmp
        chef.bmp
        chef_big.bmp
        chiche.bmp
        chiche_big.bmp
        intro.csf
        seine_w.bmp
        test)

add_executable(hunter ${SOURCE})
target_link_libraries(hunter stdk stdc bin-entry)

add_custom_command(TARGET hunter
        POST_BUILD
        DEPENDS mkkfs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating hunter rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o ${CMAKE_CURRENT_BINARY_DIR}/hunter.rom -n hunter ${FILES})
