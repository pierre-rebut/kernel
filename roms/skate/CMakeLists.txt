set(SOURCE skate.c skater.c gfx.c park.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/skate
        ${CMAKE_CURRENT_SOURCE_DIR}/box.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/boxup.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/crash.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/flip1.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/flip2.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/flip3.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/flip4.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/flip5.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/logo.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/rail.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/rotate1.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/rotate2.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/rotate3.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/rotate4.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/rotate5.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/skater1.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/skater2.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/skater3.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/skater4.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/skater5.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/skater6.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/skaterup.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/slide1.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/slide2.bmp)

add_executable(skate ${SOURCE})
target_link_libraries(skate k c)

add_custom_command(TARGET skate
        POST_BUILD
        DEPENDS mkkfs
        COMMENT "Creating skate rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o skate.rom -n skate ${FILES})


