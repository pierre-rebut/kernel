set(SOURCE vaders.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/vaders
        ball.bmp
        ball.csf
        chef.bmp
        chef_big.bmp
        chiche.bmp
        chiche_big.bmp
        intro.csf
        seine_w.bmp)

add_executable(vaders ${SOURCE})
target_link_libraries(vaders stdc)

add_custom_command(TARGET vaders
        POST_BUILD
        DEPENDS mkkfs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating vaders rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o ${CMAKE_CURRENT_BINARY_DIR}/vaders.rom -n vaders ${FILES})


