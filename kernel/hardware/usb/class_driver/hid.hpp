/**
 * @file usb/class_driver/hid.hpp
 *
 * @brief HID class driver
 *
 * This file defines the `hid_driver` class, which is a driver for USB Human
 * Interface Device (HID) class. It manages communication and data processing for
 * devices like keyboards, mice, and other human interface devices.
 *
 */

#pragma once

#include "base.hpp"

#include <array>

namespace usb
{
class hid_driver : public class_driver
{
public:
	hid_driver(device* dev, int interface_index, int in_packet_size);

	void initialize() override;
	void set_endpoint(const endpoint_config& config) override;
	void on_endpoints_configured() override;
	void on_control_completed(endpoint_id ep_id,
							  setup_stage_data* setup_data,
							  void* buf,
							  int len) override;
	void on_interrupt_completed(endpoint_id ep_id, void* buf, int len) override;

	virtual void on_data_received() = 0;
	const static size_t BUFFER_SIZE = 1024;
	const std::array<uint8_t, BUFFER_SIZE>& buffer() const { return buffer_; }
	const std::array<uint8_t, BUFFER_SIZE>& prev_buffer() const
	{
		return prev_buffer_;
	}

private:
	endpoint_id ep_interrupt_in_;
	endpoint_id ep_interrupt_out_;
	const int interface_index_;
	int in_packet_size_;
	int initialized_stage_{ 0 };

	std::array<uint8_t, BUFFER_SIZE> buffer_{}, prev_buffer_{};
};
} // namespace usb