set(VIRTIO_SOURCE_FILES
        virtio.cpp
        pci.cpp
        )

add_library(UchosVirtio ${VIRTIO_SOURCE_FILES})
