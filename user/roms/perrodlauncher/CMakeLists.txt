set(SOURCE perrodlauncher.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/perrodlauncher
        1.bmp
        2.bmp
        3.bmp
        4.bmp
        5.bmp
        6.bmp
        7.bmp
        8.bmp
        ball.csf
        intro.csf
        nuage.bmp
        sol.bmp
        yaka.bmp)

add_executable(perrodlauncher ${SOURCE})
target_link_libraries(perrodlauncher stdc)

add_custom_command(TARGET perrodlauncher
        POST_BUILD
        DEPENDS mkkfs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating perrodlauncher rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o ${CMAKE_CURRENT_BINARY_DIR}/perrodlauncher.rom -n perrodlauncher ${FILES})


