set(XHCI_SOURCE_FILES
        device.cpp
        ring.cpp
        trb.cpp
        xhci.cpp
        )

add_library(UchosXhci ${XHCI_SOURCE_FILES})