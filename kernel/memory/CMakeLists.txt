set(MEMORY_SOURCE_FILES
    page.cpp
    paging.cpp
    page_operations.asm
    segments_operations.asm
    slab.cpp
    bootstrap_allocator.cpp
    segment.cpp
    buddy_system.cpp
)

add_library(UchosMemory ${MEMORY_SOURCE_FILES})


