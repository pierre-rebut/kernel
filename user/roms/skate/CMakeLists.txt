set(SOURCE skate.c skater.c gfx.c park.c)
set(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/skate
        box.bmp
        boxup.bmp
        crash.bmp
        flip1.bmp
        flip2.bmp
        flip3.bmp
        flip4.bmp
        flip5.bmp
        logo.bmp
        rail.bmp
        rotate1.bmp
        rotate2.bmp
        rotate3.bmp
        rotate4.bmp
        rotate5.bmp
        skater1.bmp
        skater2.bmp
        skater3.bmp
        skater4.bmp
        skater5.bmp
        skater6.bmp
        skaterup.bmp
        slide1.bmp
        slide2.bmp)

add_executable(skate ${SOURCE})
target_link_libraries(skate stdk stdc crt0)

add_custom_command(TARGET skate
        POST_BUILD
        DEPENDS mkkfs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating skate rom"
        COMMAND ${CMAKE_BINARY_DIR}/tools/mkkfs/mkkfs -o ${CMAKE_CURRENT_BINARY_DIR}/skate.rom -n skate ${FILES})


