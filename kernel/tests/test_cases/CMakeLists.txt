set(TESTCASES_SOURCE_FILES
        bit_utils_test.cpp
        memory_test.cpp
        paging_test.cpp
        user_test.cpp
        task_test.cpp
        timer_test.cpp
        virtio_blk_test.cpp
        stdio_test.cpp
        fs_test.cpp
        fd_test.cpp
        graphics_test.cpp
        heap_debug_test.cpp
)

add_library(UchosTestCases ${TESTCASES_SOURCE_FILES})
