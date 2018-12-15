cmake_minimum_required(VERSION 3.8)
project(kernel)

set(CMAKE_C_FLAGS "-std=gnu99 -Os -Wall -Wextra -fno-builtin -ffreestanding -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -fno-asynchronous-unwind-tables -fno-common -static -fno-stack-protector")

set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -m32 -Wl,--build-id=none -nostartfiles")

add_subdirectory(k)

add_subdirectory(libs/libc)
add_subdirectory(libs/libk)

add_subdirectory(tools/mkkfs)
add_subdirectory(tools/mkksf)

add_subdirectory(roms)

add_custom_target(k.iso ALL
        COMMAND ${CMAKE_SOURCE_DIR}/tools/create-iso.sh k.iso kernel_epita .
        DEPENDS kernel_epita hunter skate pong vaders yakanoid perrodlauncher
        )