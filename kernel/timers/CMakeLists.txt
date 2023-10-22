set(TIMERS_SOURCE_FILES 
    acpi.cpp
    local_apic.cpp
    timer.cpp
)

add_library(UchosTimers ${TIMERS_SOURCE_FILES})