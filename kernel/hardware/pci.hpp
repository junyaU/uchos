#pragma once

#include <array>
#include <cstdint>

namespace pci
{
const uint16_t CONFIG_ADDRESS_PORT = 0x0cf8;
const uint16_t CONFIG_DATA_PORT = 0x0cfc;

struct class_code {
	uint8_t base, sub, interface;
};

struct device {
	uint8_t bus, device, function, header_type;
	class_code class_code;
};

inline std::array<device, 32> devices;

void read_function(uint8_t bus, uint8_t device, uint8_t func);
void read_device(uint8_t bus, uint8_t device);
void read_bus(uint8_t bus);

} // namespace pci

void initialize_pci();