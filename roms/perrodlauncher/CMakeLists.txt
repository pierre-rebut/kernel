set(SOURCE perrodlauncher.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/perrodlauncher
        ${CMAKE_CURRENT_SOURCE_DIR}/1.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/2.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/3.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/4.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/5.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/6.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/7.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/8.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/ball.csf
        ${CMAKE_CURRENT_SOURCE_DIR}/intro.csf
        ${CMAKE_CURRENT_SOURCE_DIR}/nuage.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/sol.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/yaka.bmp)

add_executable(perrodlauncher ${SOURCE})
target_link_libraries(perrodlauncher k c)

add_custom_command(TARGET perrodlauncher
        POST_BUILD
        DEPENDS mkkfs
        COMMENT "Creating perrodlauncher rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o perrodlauncher.rom -n perrodlauncher ${FILES})


