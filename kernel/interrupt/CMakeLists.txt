set(INTERRUPT_SOURCE_FILES
        fault.cpp
        handler.asm
        handlers.cpp
        idt.cpp
)

add_library(UchosInterrupt ${INTERRUPT_SOURCE_FILES})
