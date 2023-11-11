#pragma once

namespace usb
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

constexpr endpoint_id default_control_pipe_id = endpoint_id(0, true);

struct endpoint_config {
	endpoint_id id;
	endpoint_type type;
	int max_packet_size;
	int interval;
};

} // namespace usb
