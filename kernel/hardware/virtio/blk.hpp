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

#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

namespace kernel::hw::virtio {

// Forward declaration
struct virtio_pci_device;

/**
 * @brief VirtIO block device feature bits
 * 
 * These constants define the feature bits that can be negotiated
 * between the driver and device during initialization.
 * @see https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-3080003
 * @{
 */
constexpr int VIRTIO_BLK_F_SIZE_MAX = 1;        ///< Maximum size of any single segment
constexpr int VIRTIO_BLK_F_SEG_MAX = 2;         ///< Maximum number of segments in request
constexpr int VIRTIO_BLK_F_GEOMETRY = 4;        ///< Legacy geometry (CHS) available
constexpr int VIRTIO_BLK_F_RO = 5;              ///< Device is read-only
constexpr int VIRTIO_BLK_F_BLK_SIZE = 6;        ///< Block size of disk is available
constexpr int VIRTIO_BLK_F_FLUSH = 9;           ///< Cache flush command support
constexpr int VIRTIO_BLK_F_TOPOLOGY = 10;       ///< Topology information is available
constexpr int VIRTIO_BLK_F_CONFIG_WCE = 11;     ///< Write cache enable/disable support
constexpr int VIRTIO_BLK_F_DISCARD = 13;        ///< Discard command support
constexpr int VIRTIO_BLK_F_WRITE_ZEROES = 14;   ///< Write zeroes command support
constexpr int VIRTIO_BLK_F_LIFE_TIME = 15;      ///< Device lifetime information available
constexpr int VIRTIO_BLK_F_SECURE_ERASE = 16;   ///< Secure erase command support
constexpr int VIRTIO_BLK_F_ZONED = 17;          ///< Zoned block device support
/** @} */

/**
 * @brief VirtIO block request types
 * 
 * These constants define the types of operations that can be
 * requested from the block device.
 * @see https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-3160006
 * @{
 */
constexpr int VIRTIO_BLK_T_IN = 0;            ///< Read data from device
constexpr int VIRTIO_BLK_T_OUT = 1;           ///< Write data to device
constexpr int VIRTIO_BLK_T_FLUSH = 4;         ///< Flush device write cache
constexpr int VIRTIO_BLK_T_GET_ID = 8;        ///< Get device ID string
constexpr int VIRTIO_BLK_T_LIFE_TIME = 10;    ///< Get device lifetime info
constexpr int VIRTIO_BLK_T_DISCARD = 11;      ///< Discard/trim sectors
constexpr int VIRTIO_BLK_T_WRITE_ZEROES = 13; ///< Write zeroes to sectors
constexpr int VIRTIO_BLK_T_SECURE_ERASE = 14; ///< Secure erase sectors
/** @} */

/**
 * @brief VirtIO block request status codes
 * 
 * These constants define the possible status values returned
 * by the device after processing a request.
 * @see https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.html#x1-3160006
 * @{
 */
constexpr int VIRTIO_BLK_S_OK = 0;      ///< Request completed successfully
constexpr int VIRTIO_BLK_S_IOERR = 1;   ///< Device or driver error
constexpr int VIRTIO_BLK_S_UNSUPP = 2;  ///< Request not supported
/** @} */

/**
 * @brief Standard sector size in bytes
 */
constexpr size_t SECTOR_SIZE = 512;

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
	uint8_t status;
} __attribute__((packed));

/**
 * @brief Global VirtIO block device instance
 */
extern virtio_pci_device* blk_dev;

/**
 * @brief Write data to the block device
 * 
 * @param buffer Buffer containing data to write
 * @param sector Starting sector number
 * @param len Number of bytes to write
 * @return error_t Error code (kSuccess on success)
 * 
 * @note len should be a multiple of SECTOR_SIZE
 */
error_t write_to_blk_device(const char* buffer, uint64_t sector, uint32_t len);

/**
 * @brief Read data from the block device
 * 
 * @param buffer Buffer to store read data
 * @param sector Starting sector number
 * @param len Number of bytes to read
 * @return error_t Error code (kSuccess on success)
 * 
 * @note len should be a multiple of SECTOR_SIZE
 */
error_t read_from_blk_device(const char* buffer, uint64_t sector, uint32_t len);

/**
 * @brief Initialize the VirtIO block device
 * 
 * Performs device discovery, feature negotiation, and queue setup.
 * 
 * @return error_t Error code (kSuccess on success)
 */
error_t init_blk_device();

/**
 * @brief VirtIO block device task handler
 * 
 * This function processes block device requests and handles
 * completion notifications. It should be called periodically
 * or in response to device interrupts.
 */
void virtio_blk_task();

} // namespace kernel::hw::virtio
