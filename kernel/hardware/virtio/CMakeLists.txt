set(VIRTIO_SOURCE_FILES
        blk.cpp
        net.cpp
        virtio.cpp
        pci.cpp
        )

add_library(UchosVirtio ${VIRTIO_SOURCE_FILES})
