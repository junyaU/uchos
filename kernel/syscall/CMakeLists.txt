set(SYSCALL_SOURCE_FILES
        entry.asm
        handler.cpp
        syscall.cpp
)

add_library(UchosSyscall ${SYSCALL_SOURCE_FILES})
