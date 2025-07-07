# FAT32 filesystem implementation
add_library(UchosFAT32 STATIC
    fat.cpp
    init.cpp
    directory.cpp
    file.cpp
    utils.cpp
)

target_include_directories(UchosFAT32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(UchosFAT32
    UchosMemory
    UchosGraphics
    UchosTask
    UchosVirtio
)
