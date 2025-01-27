set(TEST_SOURCE_FILES
        framework.cpp
)

add_library(UchosTest ${TEST_SOURCE_FILES})

add_subdirectory(test_cases)

target_link_libraries(UchosTest PRIVATE UchosTestCases)
