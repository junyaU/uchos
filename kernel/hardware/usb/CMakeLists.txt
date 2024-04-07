set(USB_SOURCE_FILES
        device.cpp
        endpoint.cpp
)

add_library(UchosUsb ${USB_SOURCE_FILES})

add_subdirectory(class_driver)
add_subdirectory(xhci)

target_link_libraries(UchosUsb PRIVATE UchosXhci UchosClassDriver)
