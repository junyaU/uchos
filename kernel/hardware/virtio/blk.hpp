#pragma once

#include <cstdint>

/* Feature bits */
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

/* Request types */
constexpr int VIRTIO_BLK_T_IN = 0;
constexpr int VIRTIO_BLK_T_OUT = 1;
constexpr int VIRTIO_BLK_T_FLUSH = 4;
constexpr int VIRTIO_BLK_T_GET_ID = 8;
constexpr int VIRTIO_BLK_T_LIFE_TIME = 10;
constexpr int VIRTIO_BLK_T_DISCARD = 11;
constexpr int VIRTIO_BLK_T_WRITE_ZEROES = 13;
constexpr int VIRTIO_BLK_T_SECURE_ERASE = 14;

constexpr int SECTOR_SIZE = 512;

struct virtio_blk_req {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
	uint8_t data[SECTOR_SIZE];
	uint8_t status;
} __attribute__((packed));
