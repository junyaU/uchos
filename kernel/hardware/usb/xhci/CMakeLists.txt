set(XHCI_SOURCE_FILES
        context.cpp
        device.cpp
        device_manager.cpp
        port.cpp
        registers.cpp
        ring.cpp
        trb.cpp
        xhci.cpp
        )

add_library(UchosXhci ${XHCI_SOURCE_FILES})