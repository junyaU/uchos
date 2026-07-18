set(TESTCASES_SOURCE_FILES
        memory_test.cpp
        task_test.cpp
        timer_test.cpp
        virtio_blk_test.cpp
        stdio_test.cpp
        fs_test.cpp
        fd_test.cpp
)

add_library(UchosTestCases ${TESTCASES_SOURCE_FILES})
