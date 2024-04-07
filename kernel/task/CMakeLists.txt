set(TASK_SOURCE_FILES
        builtin.cpp
        context_switch.asm
        task.cpp
        ipc.cpp
)

add_library(UchosTask ${TASK_SOURCE_FILES})
