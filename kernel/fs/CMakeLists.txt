add_subdirectory(fat)

set(FILE_SYSTEM_SOURCE_FILES
        file_descriptor.cpp
        file_info.cpp
        path.cpp
)

add_library(UchosFileSystem ${FILE_SYSTEM_SOURCE_FILES})

target_link_libraries(UchosFileSystem UchosFAT32)
