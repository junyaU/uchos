#include "hid.hpp"
#include <algorithm>
#include "../device.hpp"
#include "../endpoint.hpp"
#include "../setup_stage_data.hpp"

namespace kernel::hw::usb
{
HidDriver::HidDriver(Device* dev, int interface_index, int in_packet_size)
	: ClassDriver(dev),
	  interface_index_(interface_index),
	  in_packet_size_(in_packet_size)
{
}

void HidDriver::initialize() {}

void HidDriver::set_endpoint(const EndpointConfig& config)
{
	if (config.type != EndpointType::INTERRUPT) {
		return;
	}

	if (config.id.is_in()) {
		ep_interrupt_in_ = config.id;
	} else {
		ep_interrupt_out_ = config.id;
	}
}

void HidDriver::on_endpoints_configured()
{
	SetupStageData setup_data{};
	setup_data.request_type.bits.direction = request_type::OUT;
	setup_data.request_type.bits.type = request_type::CLASS;
	setup_data.request_type.bits.recipient = request_type::INTERFACE;
	setup_data.request = request::SET_PROTOCOL;
	setup_data.value = 0;
	setup_data.index = interface_index_;
	setup_data.length = 0;

	initialized_stage_ = 1;
	parent_device()->control_out(
			{ DEFAULT_CONTROL_PIPE_ID, setup_data, nullptr, 0, this });
}

void HidDriver::on_control_completed(EndpointId ep_id,
									  SetupStageData setup_data,
									  void* buf,
									  int len)
{
	if (initialized_stage_ == 1) {
		initialized_stage_ = 2;

		parent_device()->interrupt_in(
				{ ep_interrupt_in_, buffer_.data(), in_packet_size_ });
	}
}

void HidDriver::on_interrupt_completed(EndpointId ep_id, void* buf, int len)
{
	if (ep_id.is_in()) {
		on_data_received();
		std::copy_n(buffer_.begin(), len, prev_buffer_.begin());

		// prepare for next interrupt
		parent_device()->interrupt_in(
				{ ep_interrupt_in_, buffer_.data(), in_packet_size_ });
	}
}
} // namespace kernel::hw::usb
