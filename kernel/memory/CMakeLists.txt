set(MEMORY_SOURCE_FILES
    page.cpp
    paging.cpp
    paging_utils.asm
    segment_utils.asm
    slab.cpp
    bootstrap_allocator.cpp
    segment.cpp
    buddy_system.cpp
    user.cpp
)

add_library(UchosMemory ${MEMORY_SOURCE_FILES})
