set(XHCI_SOURCE_FILES
        xhci.cpp
        ring.cpp
        )

add_library(UchosXhci ${XHCI_SOURCE_FILES})