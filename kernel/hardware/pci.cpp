#include "pci.hpp"
#include "../asm_utils.h"
#include "../graphics/kernel_logger.hpp"
namespace pci
{
uint32_t calc_config_addr(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
	return (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (offset & 0xfcU);
}

uint8_t read_header_type(uint8_t bus, uint8_t device, uint8_t func)
{
	write_to_io_port(CONFIG_ADDRESS_PORT, calc_config_addr(bus, device, func, 0x0c));
	return (read_from_io_port(CONFIG_DATA_PORT) >> 16) & 0xffU;
}

uint16_t read_vendor_id(uint8_t bus, uint8_t device, uint8_t func)
{
	write_to_io_port(CONFIG_ADDRESS_PORT, calc_config_addr(bus, device, func, 0x00));
	return read_from_io_port(CONFIG_DATA_PORT) & 0xffffU;
}

uint16_t read_device_id(uint8_t bus, uint8_t device, uint8_t func)
{
	write_to_io_port(CONFIG_ADDRESS_PORT, calc_config_addr(bus, device, func, 0x00));
	return (read_from_io_port(CONFIG_DATA_PORT) >> 16);
}

class_code read_class_code(uint8_t bus, uint8_t device, uint8_t func)
{
	write_to_io_port(CONFIG_ADDRESS_PORT, calc_config_addr(bus, device, func, 0x08));
	auto reg = read_from_io_port(CONFIG_DATA_PORT);

	class_code cc;
	cc.base = (reg >> 24) & 0xffU;
	cc.sub = (reg >> 16) & 0xffU;
	cc.interface = (reg >> 8) & 0xffU;

	return cc;
}

bool is_single_function_device(uint8_t header_type)
{
	return (header_type & 0x80U) == 0;
}

void read_function(uint8_t bus, uint8_t dev, uint8_t func)
{
	auto header_type = read_header_type(bus, dev, func);
	auto class_code = read_class_code(bus, dev, func);

	device d;
	d.bus = bus;
	d.device = dev;
	d.function = func;
	d.header_type = header_type;
	d.class_code = class_code;
	d.vendor_id = read_vendor_id(bus, dev, func);

	devices[dev] = d;
	num_devices++;
}

void read_device(uint8_t bus, uint8_t device)
{
	if (is_single_function_device(read_header_type(bus, device, 0))) {
		read_function(bus, device, 0);
		return;
	}

	for (uint8_t func = 0; func < 8; func++) {
		if (read_vendor_id(bus, device, func) == 0xffffU) {
			continue;
		}

		read_function(bus, device, func);
	}
}

void read_bus(uint8_t bus)
{
	for (uint8_t device = 0; device < 32; device++) {
		if (read_vendor_id(bus, device, 0) == 0xffffU) {
			continue;
		}

		read_device(bus, device);
	}
}

void load_devices()
{
	auto host_bridge_header_type = read_header_type(0, 0, 0);

	if (pci::is_single_function_device(host_bridge_header_type)) {
		pci::read_bus(0);
		return;
	}

	for (uint8_t func = 0; func < 8; func++) {
		if (pci::read_vendor_id(0, 0, func) == 0xffffU) {
			continue;
		}

		pci::read_bus(func);
	}
}

uint64_t read_base_address_register(device& dev, unsigned int index)
{
	const auto base_addr_index = 0x10 + index * 4;
	write_to_io_port(
			CONFIG_ADDRESS_PORT,
			calc_config_addr(dev.bus, dev.device, dev.function, base_addr_index));

	const auto bar_low = read_from_io_port(CONFIG_DATA_PORT);

	if ((bar_low & 4) == 0) {
		return bar_low;
	}

	write_to_io_port(CONFIG_ADDRESS_PORT,
					 calc_config_addr(dev.bus, dev.device, dev.function,
									  base_addr_index + 4));
	const auto bar_high = read_from_io_port(CONFIG_DATA_PORT);

	return bar_low | (static_cast<uint64_t>(bar_high) << 32);
}

} // namespace pci

void initialize_pci()
{
	pci::load_devices();
	//	for (int i = 0; i < pci::num_devices; i++) {
	//		auto d = pci::devices[i];
	//
	//		klogger->printf(
	//				"bus: %d, device: %d, function: %d, vendor_id: %d, class_code:
	//%d\n", 				d.bus, d.device, d.function, d.vendor_id, d.class_code.base);
	//	}
}
