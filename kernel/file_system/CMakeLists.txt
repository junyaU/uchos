set(FILE_SYSTEM_SOURCE_FILES
        fat.cpp
        file_descriptor.cpp
        file_info.cpp
        path.cpp
)

add_library(UchosFileSystem ${FILE_SYSTEM_SOURCE_FILES})
