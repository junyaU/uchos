set(GRAPHICS_SOURCE_FILES
        color.cpp
        font.cpp
        log.cpp
        screen.cpp
        hankaku.o
        )

set(TEMP_DIR ${CMAKE_BINARY_DIR}/temp_objcopy)

add_custom_command(
        OUTPUT hankaku.o
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TEMP_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/hankaku.bin ${TEMP_DIR}/
        COMMAND cd ${TEMP_DIR} && objcopy -I binary -O elf64-x86-64 -B i386:x86-64 hankaku.bin ${CMAKE_CURRENT_BINARY_DIR}/hankaku.o
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${TEMP_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hankaku.bin
        COMMENT "Converting hankaku.bin to hankaku.o"
)

add_library(UchosGraphics ${GRAPHICS_SOURCE_FILES})
