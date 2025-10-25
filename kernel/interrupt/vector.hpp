/**
 * @file vector.hpp
 * @brief Interrupt vector definitions for hardware and software interrupts
 */

#pragma once

namespace kernel::interrupt
{

/**
 * @brief Interrupt vector management class
 *
 * This class defines the interrupt vector numbers used by the kernel for
 * various hardware and software interrupts. These vectors are programmed
 * into the Interrupt Descriptor Table (IDT).
 */
class InterruptVector
{
public:
	/**
	 * @brief Interrupt vector numbers
	 *
	 * Defines specific interrupt vector numbers for various interrupt sources.
	 * These values are chosen to avoid conflicts with CPU exception vectors (0-31)
	 * and standard ISA IRQs (32-47).
	 */
	enum Number {
		/**
		 * @brief Local APIC timer interrupt vector
		 *
		 * Used for the Local APIC timer, which provides high-resolution
		 * timing and scheduling capabilities.
		 */
		LOCAL_APIC_TIMER = 0x40,
		/**
		 * @brief XHCI (USB 3.0) controller interrupt vector
		 *
		 * Used for interrupts from the eXtensible Host Controller Interface,
		 * which manages USB 3.0 devices.
		 */
		XHCI = 0x41,
		/**
		 * @brief VirtIO block device interrupt vector
		 *
		 * Used for interrupts from VirtIO block devices in virtualized environments.
		 */
		VIRTIO_BLK = 0x42,
		/**
		 * @brief VirtIO block device queue interrupt vector
		 *
		 * Used for queue-specific interrupts from VirtIO block devices.
		 */
		VIRTQUEUE_BLK = 0x43,
		/**
		 * @brief VirtIO network device interrupt vector
		 *
		 * Used for interrupts from VirtIO network devices in virtualized
		 * environments.
		 */
		VIRTIO_NET = 0x44,
		/**
		 * @brief VirtIO network device interrupt vector
		 *
		 * Used for interrupts from VirtIO network devices in virtualized
		 * environments.
		 */
		VIRTQUEUE_NET = 0x45,
		/**
		 * @brief Task switching interrupt vector
		 *
		 * Software interrupt used to trigger context switches between tasks.
		 * This allows cooperative task switching without waiting for the
		 * timer interrupt.
		 */
		SWITCH_TASK = 0x46
	};
};

} // namespace kernel::interrupt
