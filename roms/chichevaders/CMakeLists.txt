set(SOURCE vaders.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/vaders
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.csf
        ${CMAKE_CURRENT_SOURCE_DIR}/chef.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/chef_big.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/chiche.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/chiche_big.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/intro.csf
        ${CMAKE_CURRENT_SOURCE_DIR}/seine_w.bmp)

add_executable(vaders ${SOURCE})
target_link_libraries(vaders k c bin-entry)

add_custom_command(TARGET vaders
        POST_BUILD
        DEPENDS mkkfs
        COMMENT "Creating vaders rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o vaders.rom -n vaders ${FILES})


