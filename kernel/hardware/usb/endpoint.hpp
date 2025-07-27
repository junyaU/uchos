/**
 * @file hardware/usb/endpoint.hpp
 *
 * @brief USB endpoint
 *
 * This file provides classes and functions to handle USB endpoints, including
 * definitions of endpoint types, endpoint identifiers, and utilities to create
 * endpoint configurations from USB endpoint descriptors.
 *
 */

#pragma once

#include "descriptor.hpp"

namespace kernel::hw::usb
{
enum class EndpointType {
	CONTROL = 0,
	ISOCHRONOUS = 1,
	BULK = 2,
	INTERRUPT = 3,
};

class EndpointId
{
public:
	constexpr EndpointId() : addr_(0) {}
	constexpr EndpointId(const EndpointId& id) : addr_(id.addr_) {}
	explicit constexpr EndpointId(int addr) : addr_(addr) {}

	constexpr EndpointId(int ep_num, bool in) : addr_(ep_num << 1 | in) {}

	EndpointId& operator=(const EndpointId& id)
	{
		addr_ = id.addr_;
		return *this;
	}

	int address() const { return addr_; }

	int number() const { return addr_ >> 1; }

	bool is_in() const { return addr_ & 1; }

private:
	int addr_;
};

constexpr EndpointId DEFAULT_CONTROL_PIPE_ID = EndpointId(0, true);

struct EndpointConfig {
	EndpointId id;
	EndpointType type;
	int max_packet_size;
	int interval;
};

EndpointConfig make_endpoint_config(const EndpointDescriptor& desc);

} // namespace kernel::hw::usb
