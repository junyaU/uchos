set(HARDWARE_SOURCE_FILES
        keyboard.cpp
        pci.cpp
        )

add_library(UchosHardware ${HARDWARE_SOURCE_FILES})

add_subdirectory(usb)
add_subdirectory(virtio)

target_link_libraries(UchosHardware PRIVATE UchosUsb UchosVirtio)
