set(USB_SOURCE_FILES
        memory.cpp
        )

add_library(UchosUsb ${USB_SOURCE_FILES})

add_subdirectory(xhci)

target_link_libraries(UchosUsb PRIVATE UchosXhci)
