#include "pci.hpp"
#include "asm_utils.h"
#include "hardware/mm_register.hpp"
#include <cstddef>
#include <cstdint>

namespace kernel::hw::pci
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

uint32_t read_conf_reg(const device& dev, uint8_t reg_addr)
{
	write_to_io_port(CONFIG_ADDRESS_PORT,
					 calc_config_addr(dev.bus, dev.device, dev.function, reg_addr));
	return read_from_io_port(CONFIG_DATA_PORT);
}

void write_conf_reg(const device& dev, uint8_t reg_addr, uint32_t value)
{
	write_to_io_port(CONFIG_ADDRESS_PORT,
					 calc_config_addr(dev.bus, dev.device, dev.function, reg_addr));
	write_to_io_port(CONFIG_DATA_PORT, value);
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
	d.device_id = read_device_id(bus, dev, func);

	devices[dev] = d;
	++num_devices;
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

	if (is_single_function_device(host_bridge_header_type)) {
		read_bus(0);
		return;
	}

	for (uint8_t func = 0; func < 8; func++) {
		if (read_vendor_id(0, 0, func) == 0xffffU) {
			continue;
		}

		read_bus(func);
	}
}

uint64_t read_base_address_register(const device& dev, unsigned int index)
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

uint8_t get_capability_pointer(const device& dev)
{
	return read_conf_reg(dev, 0x34) & 0xffU;
}

msi_capability read_msi_capability(const device& dev, uint8_t cap_addr)
{
	msi_capability msi_cap{};
	msi_cap.header.data = read_conf_reg(dev, cap_addr);
	msi_cap.msg_addr = read_conf_reg(dev, cap_addr + 4);

	uint8_t msg_data_addr = cap_addr + 8;
	if (msi_cap.header.bits.addr_64_capable) {
		msi_cap.msg_upper_addr = read_conf_reg(dev, cap_addr + 8);
		msg_data_addr = cap_addr + 12;
	}

	msi_cap.msg_data = read_conf_reg(dev, msg_data_addr);

	if (msi_cap.header.bits.per_vector_mask_capable) {
		msi_cap.mask_bits = read_conf_reg(dev, msg_data_addr + 4);
		msi_cap.pending_bits = read_conf_reg(dev, msg_data_addr + 8);
	}

	return msi_cap;
}

msi_x_capability read_msi_x_capability(const device& dev, uint8_t cap_addr)
{
	msi_x_capability msix_cap{};

	msix_cap.header.data = read_conf_reg(dev, cap_addr);
	msix_cap.table.data = read_conf_reg(dev, cap_addr + 4);
	msix_cap.pba.data = read_conf_reg(dev, cap_addr + 8);

	return msix_cap;
}

void write_msi_capability(const device& dev,
						  uint8_t cap_addr,
						  const msi_capability& msi_cap)
{
	write_conf_reg(dev, cap_addr, msi_cap.header.data);
	write_conf_reg(dev, cap_addr + 4, msi_cap.msg_addr);

	uint8_t msg_data_addr = cap_addr + 8;
	if (msi_cap.header.bits.addr_64_capable) {
		write_conf_reg(dev, cap_addr + 8, msi_cap.msg_upper_addr);
		msg_data_addr = cap_addr + 12;
	}

	write_conf_reg(dev, msg_data_addr, msi_cap.msg_data);

	if (msi_cap.header.bits.per_vector_mask_capable) {
		write_conf_reg(dev, msg_data_addr + 4, msi_cap.mask_bits);
		write_conf_reg(dev, msg_data_addr + 8, msi_cap.pending_bits);
	}
}

void configure_msi_x_table_entry(const device& dev,
								 const msi_x_capability& msix_cap,
								 uint8_t cap_addr,
								 uint32_t msg_addr,
								 uint32_t msg_data)
{
	uint64_t bar_addr = read_base_address_register(dev, msix_cap.table.bits.bar);
	bar_addr &= ~0xfff;
	bar_addr += msix_cap.table.bits.offset << 3;
	auto* table_entry = reinterpret_cast<msix_table_entry*>(bar_addr);

	for (size_t i = 0; i <= msix_cap.header.bits.size_of_table; ++i) {
		if (table_entry[i].msg_addr.read() != 0) {
			continue;
		}

		table_entry[i].msg_addr.write(default_bitmap<uint32_t>{ msg_addr });
		table_entry[i].msg_upper_addr.write(default_bitmap<uint32_t>{ 0 });
		table_entry[i].msg_data.write(default_bitmap<uint32_t>{ msg_data });
		table_entry[i].vector_control.write(default_bitmap<uint32_t>{ 0 });

		break;
	}

	// Memory barrier to ensure writes are not reordered
	asm volatile("mfence" ::: "memory");
}

void configure_msi_register(const device& dev,
							uint8_t cap_addr,
							uint32_t msg_addr,
							uint32_t msg_data,
							unsigned int num_vector_exponent)
{
	auto msi_cap = read_msi_capability(dev, cap_addr);

	if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent) {
		msi_cap.header.bits.multi_msg_enable = msi_cap.header.bits.multi_msg_capable;
	} else {
		msi_cap.header.bits.multi_msg_enable = num_vector_exponent;
	}

	msi_cap.header.bits.msi_enable = 1;
	msi_cap.msg_addr = msg_addr;
	msi_cap.msg_data = msg_data;

	write_msi_capability(dev, cap_addr, msi_cap);
}

void configure_msi_x_register(const device& dev,
							  uint8_t cap_addr,
							  uint32_t msg_addr,
							  uint32_t msg_data)
{
	auto msix_cap = read_msi_x_capability(dev, cap_addr);

	msix_cap.header.bits.enable = 1;
	msix_cap.header.bits.function_mask = 0;

	write_conf_reg(dev, cap_addr, msix_cap.header.data);

	configure_msi_x_table_entry(dev, msix_cap, cap_addr, msg_addr, msg_data);
}

capability_header read_capability_header(const device& dev, uint8_t addr)
{
	capability_header header;
	header.data = read_conf_reg(dev, addr);
	return header;
}

void configure_msi(const device& dev,
				   uint32_t msg_addr,
				   uint32_t msg_data,
				   unsigned int num_vector_exponent)
{
	uint8_t capability_pointer = get_capability_pointer(dev);
	uint8_t msi_capability_addr = 0, msix_capability_addr = 0;
	while (capability_pointer != 0) {
		auto header = read_capability_header(dev, capability_pointer);
		if (header.bits.cap_id == CAP_MSI) {
			msi_capability_addr = capability_pointer;
		} else if (header.bits.cap_id == CAP_MSIX) {
			msix_capability_addr = capability_pointer;
		}

		capability_pointer = header.bits.next_ptr;
	}

	if (msix_capability_addr != 0) {
		configure_msi_x_register(dev, msix_capability_addr, msg_addr,
										msg_data);
		return;
	}

	if (msi_capability_addr != 0) {
		configure_msi_register(dev, msi_capability_addr, msg_addr, msg_data,
									  num_vector_exponent);
		return;
	}
}

void configure_msi_fixed_destination(const device& dev,
									 uint8_t apic_id,
									 msi_trigger_mode trigger_mode,
									 msi_delivery_mode delivery_mode,
									 uint8_t vector,
									 unsigned int num_vector_exponent)
{
	const uint32_t msg_addr = 0xfee00000U | (apic_id << 12);
	uint32_t msg_data = (static_cast<uint32_t>(delivery_mode) << 8) | vector;
	if (trigger_mode == msi_trigger_mode::LEVEL) {
		msg_data |= 0xc000;
	}

	configure_msi(dev, msg_addr, msg_data, num_vector_exponent);
}

void initialize() { load_devices(); }

} // namespace kernel::hw::pci
