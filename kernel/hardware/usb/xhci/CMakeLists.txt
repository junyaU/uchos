set(XHCI_SOURCE_FILES
        device.cpp
        device_manager.cpp
        ring.cpp
        trb.cpp
        xhci.cpp
        )

add_library(UchosXhci ${XHCI_SOURCE_FILES})