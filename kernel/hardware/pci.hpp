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

} // namespace pci

void initialize_pci();