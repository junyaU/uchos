set(VIRTIO_SOURCE_FILES
        blk.cpp
        device_descriptor.cpp
        net.cpp
        virtio.cpp
        pci.cpp
        )

add_library(UchosVirtio ${VIRTIO_SOURCE_FILES})
