set(INTERRUPT_SOURCE_FILES
    handler.cpp
    idt.cpp
)

add_library(UchosInterrupt ${INTERRUPT_SOURCE_FILES})
