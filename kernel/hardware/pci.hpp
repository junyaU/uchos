/**
 * @file hardware/pci.hpp
 *
 * @brief PCI (Peripheral Component Interconnect) Device Driver
 *
 * This file defines the structures, functions, and constants used for
 * interacting with PCI devices. It provides functionalities for enumerating
 * PCI devices on the bus, reading their configuration space, and managing
 * capabilities such as MSI (Message Signaled Interrupts).
 *
 */

#pragma once

#include <array>
#include <cstdint>

namespace pci
{
const uint16_t CONFIG_ADDRESS_PORT = 0x0cf8;
const uint16_t CONFIG_DATA_PORT = 0x0cfc;

struct class_code {
	uint8_t base, sub, interface;

	bool match(uint8_t b) const { return b == base; }
	bool match(uint8_t b, uint8_t s) const { return match(b) && s == sub; }
	bool match(uint8_t b, uint8_t s, uint8_t i) const
	{
		return match(b, s) && i == interface;
	}
};

struct device {
	uint8_t bus, device, function, header_type;
	class_code class_code;
	uint16_t vendor_id;

	bool is_xhci() const { return class_code.match(0x0cU, 0x03U, 0x30U); }

	bool is_intel() const { return vendor_id == 0x8086; }
};

uint64_t read_base_address_register(device& dev, unsigned int index);

inline std::array<device, 32> devices;
inline int num_devices;

union capability_header {
	uint32_t data;

	struct {
		uint32_t cap_id : 8;
		uint32_t next_ptr : 8;
		uint32_t cap : 16;
	} __attribute__((packed)) bits;
} __attribute__((packed));

struct msi_capability {
	union {
		uint32_t data;

		struct {
			uint32_t cap_id : 8;
			uint32_t next_ptr : 8;
			uint32_t msi_enable : 1;
			uint32_t multi_msg_capable : 3;
			uint32_t multi_msg_enable : 3;
			uint32_t addr_64_capable : 1;
			uint32_t per_vector_mask_capable : 1;
			uint32_t : 7;
		} __attribute__((packed)) bits;
	} __attribute__((packed)) header;

	uint32_t msg_addr;
	uint32_t msg_upper_addr;
	uint32_t msg_data;
	uint32_t mask_bits;
	uint32_t pending_bits;
} __attribute__((packed));

const uint8_t CAP_MSI = 0x05;
const uint8_t CAP_MSIX = 0x11;

enum class msi_delivery_mode {
	FIXED = 0b000,
	LOWPRI = 0b001,
	SMI = 0b010,
	NMI = 0b100,
	INIT = 0b101,
	EXTINT = 0b111,
};

enum class msi_trigger_mode {
	EDGE = 0,
	LEVEL = 1,
};

void configure_msi_fixed_destination(const device& dev,
									 uint8_t apic_id,
									 msi_trigger_mode trigger_mode,
									 msi_delivery_mode delivery_mode,
									 uint8_t vector,
									 unsigned int num_vector_exponent);

} // namespace pci

void initialize_pci();