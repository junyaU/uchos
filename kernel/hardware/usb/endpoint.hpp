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
enum class endpoint_type {
	CONTROL = 0,
	ISOCHRONOUS = 1,
	BULK = 2,
	INTERRUPT = 3,
};

class endpoint_id
{
public:
	constexpr endpoint_id() : addr_(0) {}
	constexpr endpoint_id(const endpoint_id& id) : addr_(id.addr_) {}
	explicit constexpr endpoint_id(int addr) : addr_(addr) {}

	constexpr endpoint_id(int ep_num, bool in) : addr_(ep_num << 1 | in) {}

	endpoint_id& operator=(const endpoint_id& id)
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

constexpr endpoint_id DEFAULT_CONTROL_PIPE_ID = endpoint_id(0, true);

struct endpoint_config {
	endpoint_id id;
	endpoint_type type;
	int max_packet_size;
	int interval;
};

endpoint_config make_endpoint_config(const endpoint_descriptor& desc);

} // namespace kernel::hw::usb
