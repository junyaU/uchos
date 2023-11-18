/**
 * @file usb/descriptor.hpp
 *
 * @brief USB descriptor
 *
 * This file defines the structures representing various USB descriptors.
 * USB devices use these descriptors to communicate their characteristics
 * and requirements to the host controller. The host system utilizes this
 * information to initialize the device and assign appropriate drivers.
 *
 */

#pragma once

#include <cstdint>

namespace usb
{
struct device_descriptor {
	static const uint8_t TYPE = 1;

	uint8_t length;
	uint8_t type;
	uint16_t usb_release;
	uint8_t device_class;
	uint8_t device_subclass;
	uint8_t device_protocol;
	uint8_t max_packet_size;
	uint16_t vendor_id;
	uint16_t product_id;
	uint16_t device_release;
	uint8_t manufacturer;
	uint8_t product;
	uint8_t serial_number;
	uint8_t num_configurations;
} __attribute__((packed));

struct configuration_descriptor {
	static const uint8_t TYPE = 2;

	uint8_t length;
	uint8_t type;
	uint16_t total_length;
	uint8_t num_interfaces;
	uint8_t configuration_value;
	uint8_t configuration;
	uint8_t attributes;
	uint8_t max_power;
} __attribute__((packed));

struct interface_descriptor {
	static const uint8_t TYPE = 4;

	uint8_t length;
	uint8_t type;
	uint8_t interface_number;
	uint8_t alternate_setting;
	uint8_t num_endpoints;
	uint8_t interface_class;
	uint8_t interface_subclass;
	uint8_t interface_protocol;
	uint8_t interface_id;
} __attribute__((packed));

struct endpoint_descriptor {
	static const uint8_t TYPE = 5;

	uint8_t length;
	uint8_t type;

	union {
		uint8_t data;

		struct {
			uint8_t number : 4;
			uint8_t : 3;
			uint8_t direction_in : 1;
		} __attribute__((packed)) bits;
	} endpoint_address;

	union {
		uint8_t data;

		struct {
			uint8_t transfer_type : 2;
			uint8_t sync_type : 2;
			uint8_t usage_type : 2;
			uint8_t : 2;
		} __attribute__((packed)) bits;
	} attributes;

	uint16_t max_packet_size;
	uint8_t interval;
} __attribute__((packed));

} // namespace usb
