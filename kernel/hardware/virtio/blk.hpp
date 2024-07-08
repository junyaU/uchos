/**
 * @file hardware/virtio/blk.hpp
 *
 * @brief VirtIO Block Device Driver
 *
 * This file defines the structures, functions, and constants used for
 * interacting with VirtIO block devices. It provides functionalities for
 * reading from and writing to block devices, as well as initializing the
 * block device.
 */
#pragma once

#include <cstdint>
#include <libs/common/types.hpp>

// Forward declaration
struct virtio_pci_device;

// https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-3080003
constexpr int VIRTIO_BLK_F_SIZE_MAX = 1;
constexpr int VIRTIO_BLK_F_SEG_MAX = 2;
constexpr int VIRTIO_BLK_F_GEOMETRY = 4;
constexpr int VIRTIO_BLK_F_RO = 5;
constexpr int VIRTIO_BLK_F_BLK_SIZE = 6;
constexpr int VIRTIO_BLK_F_FLUSH = 9;
constexpr int VIRTIO_BLK_F_TOPOLOGY = 10;
constexpr int VIRTIO_BLK_F_CONFIG_WCE = 11;
constexpr int VIRTIO_BLK_F_DISCARD = 13;
constexpr int VIRTIO_BLK_F_WRITE_ZEROES = 14;
constexpr int VIRTIO_BLK_F_LIFE_TIME = 15;
constexpr int VIRTIO_BLK_F_SECURE_ERASE = 16;
constexpr int VIRTIO_BLK_F_ZONED = 17;

// https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-3160006
constexpr int VIRTIO_BLK_T_IN = 0;
constexpr int VIRTIO_BLK_T_OUT = 1;
constexpr int VIRTIO_BLK_T_FLUSH = 4;
constexpr int VIRTIO_BLK_T_GET_ID = 8;
constexpr int VIRTIO_BLK_T_LIFE_TIME = 10;
constexpr int VIRTIO_BLK_T_DISCARD = 11;
constexpr int VIRTIO_BLK_T_WRITE_ZEROES = 13;
constexpr int VIRTIO_BLK_T_SECURE_ERASE = 14;

// https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-3160006
constexpr int VIRTIO_BLK_S_OK = 0;
constexpr int VIRTIO_BLK_S_IOERR = 1;
constexpr int VIRTIO_BLK_S_UNSUPP = 2;

constexpr int SECTOR_SIZE = 512;

/**
 * @struct virtio_blk_req
 * @brief VirtIO Block Request Structure
 *
 * This structure represents a request to a VirtIO block device.
 *
 * @see
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-3160006
 */
struct virtio_blk_req {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
	uint8_t data[SECTOR_SIZE];
	uint8_t status;
} __attribute__((packed));

extern virtio_pci_device* blk_dev;

error_t write_to_blk_device(char* buffer, uint64_t sector, uint32_t len);

error_t read_from_blk_device(char* buffer, uint64_t sector, uint32_t len);

error_t init_blk_device();

void virtio_blk_task();
