/**
 * @file device_descriptor.hpp
 * @brief VirtIO device descriptor structures and functions
 *
 * This file defines descriptor structures for managing VirtIO device
 * interrupt configurations in a centralized and extensible manner.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "interrupt/handlers.hpp"

namespace kernel::hw::virtio
{

struct VirtioPciDevice;

/**
 * @brief Interrupt handler function type
 *
 * Function pointer type for VirtIO interrupt handlers.
 */
using interrupt_handler_t = void (*)(interrupt::InterruptFrame*);

/**
 * @brief Maximum number of interrupts per VirtIO device
 *
 * Defines the maximum number of interrupt vectors that can be
 * configured for a single VirtIO device (config + multiple queues).
 */
constexpr size_t MAX_INTERRUPTS_PER_DEVICE = 4;

/**
 * @brief VirtIO device interrupt descriptor
 *
 * Describes a single interrupt configuration for a VirtIO device,
 * including the interrupt vector number, name, and handler function.
 */
struct VirtioInterruptDescriptor {
	uint8_t vector;				 ///< Interrupt vector number
	const char* name;			 ///< Interrupt name (for debugging)
	interrupt_handler_t handler; ///< Interrupt handler function pointer
};

/**
 * @brief VirtIO device descriptor
 *
 * Contains all configuration information needed to initialize a VirtIO device.
 * New devices can be added by simply adding an entry to the descriptor array.
 *
 * @note This structure supports up to MAX_INTERRUPTS_PER_DEVICE interrupts.
 */
struct VirtioDeviceDescriptor {
	uint16_t device_id;		///< VirtIO device ID (e.g., VIRTIO_BLK, VIRTIO_NET)
	const char* name;		///< Device name (for logging)
	uint8_t num_interrupts; ///< Number of interrupts configured
	VirtioInterruptDescriptor
			interrupts[MAX_INTERRUPTS_PER_DEVICE]; ///< Interrupt configurations
};

/**
 * @brief Find VirtIO device descriptor by device type
 *
 * Searches for the device descriptor corresponding to the given device type.
 *
 * @param device_type Device type identifier (VIRTIO_BLK, VIRTIO_NET, etc.)
 * @return const VirtioDeviceDescriptor* Pointer to descriptor if found, nullptr
 * otherwise
 *
 * @example
 * const VirtioDeviceDescriptor* desc = find_virtio_device_descriptor(VIRTIO_BLK);
 * if (desc != nullptr) {
 *     // Use descriptor for device initialization
 * }
 */
const VirtioDeviceDescriptor* find_virtio_device_descriptor(int device_type);

} // namespace kernel::hw::virtio
