set(HARDWARE_SOURCE_FILES
        pci.cpp
        )

add_subdirectory(usb/xhci)

add_library(UchosHardware ${HARDWARE_SOURCE_FILES})