set(GRAPHICS_SOURCE_FILES
    color.cpp
    font.cpp
    kernel_logger.cpp
    screen.cpp
    hankaku.o
    hankaku.bin
)

add_library(UchosGraphics ${GRAPHICS_SOURCE_FILES})

add_custom_command(
    OUTPUT hankaku.o
    COMMAND objcopy -I binary -O elf64-x86-64 -B i386:x86-64 ${CMAKE_CURRENT_SOURCE_DIR}/hankaku.bin hankaku.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hankaku.bin
    COMMENT "Converting hankaku.bin to hankaku.o"
)

add_custom_target(
    ConvertHankaku ALL
    DEPENDS hankaku.o
)
