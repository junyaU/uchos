set(TASK_SOURCE_FILES
    context_switch.asm
    task_manager.cpp
    task.cpp
)

add_library(UchosTask ${TASK_SOURCE_FILES})