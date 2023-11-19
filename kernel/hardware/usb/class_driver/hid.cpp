#include "hid.hpp"

#include "../../../graphics/kernel_logger.hpp"
#include "../device.hpp"

namespace usb
{
hid_driver::hid_driver(device* dev, int interface_index, int in_packet_size)
	: class_driver(dev),
	  interface_index_(interface_index),
	  in_packet_size_(in_packet_size)
{
}

void hid_driver::initialize() {}

void hid_driver::set_endpoint(const endpoint_config& config)
{
	if (config.type != endpoint_type::INTERRUPT) {
		return;
	}

	if (config.id.is_in()) {
		ep_interrupt_in_ = config.id;
	} else {
		ep_interrupt_out_ = config.id;
	}
}

void hid_driver::on_endpoints_configured()
{
	setup_stage_data setup_data{};
	setup_data.request_type.bits.direction = request_type::OUT;
	setup_data.request_type.bits.type = request_type::CLASS;
	setup_data.request_type.bits.recipient = request_type::INTERFACE;
	setup_data.request = request::SET_PROTOCOL;
	setup_data.value = 0;
	setup_data.index = interface_index_;
	setup_data.length = 0;

	initialized_stage_ = 1;
	parent_device()->control_out(DEFAULT_CONTROL_PIPE_ID, setup_data, nullptr, 0,
								 this);
}

void hid_driver::on_control_completed(usb::endpoint_id ep_id,
									  usb::setup_stage_data* setup_data,
									  void* buf,
									  int len)
{
	klogger->info("HID driver: on_control_completed");

	if (initialized_stage_ == 1) {
		initialized_stage_ = 2;

		parent_device()->interrupt_in(ep_interrupt_in_, buffer_.data(),
									  in_packet_size_);
	}
}

void hid_driver::on_interrupt_completed(usb::endpoint_id ep_id, void* buf, int len)
{
	if (ep_id.is_in()) {
		on_data_received();
		std::copy_n(buffer_.begin(), len, prev_buffer_.begin());

		// prepare for next interrupt
		parent_device()->interrupt_in(ep_interrupt_in_, buffer_.data(),
									  in_packet_size_);
	}
}
} // namespace usb
