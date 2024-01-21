set(INTERRUPT_SOURCE_FILES
        handler.asm
        handlers.cpp
        idt.cpp
)

add_library(UchosInterrupt ${INTERRUPT_SOURCE_FILES})
