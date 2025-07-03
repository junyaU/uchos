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

namespace kernel::hw::usb
{
/**
 * @brief HID (Human Interface Device) class driver
 * 
 * This class implements the USB HID protocol for devices like keyboards,
 * mice, and other human input devices. It handles HID-specific control
 * requests and interrupt transfers.
 */
class hid_driver : public class_driver
{
public:
	/**
	 * @brief Construct a new HID driver
	 * 
	 * @param dev Parent USB device
	 * @param interface_index Index of the HID interface
	 * @param in_packet_size Maximum packet size for input endpoint
	 */
	hid_driver(device* dev, int interface_index, int in_packet_size);

	void initialize() override;
	void set_endpoint(const endpoint_config& config) override;
	void on_endpoints_configured() override;
	void on_control_completed(endpoint_id ep_id,
							  setup_stage_data setup_data,
							  void* buf,
							  int len) override;
	void on_interrupt_completed(endpoint_id ep_id, void* buf, int len) override;

	/**
	 * @brief Called when new HID data is received
	 * 
	 * Derived classes must implement this to process HID reports.
	 */
	virtual void on_data_received() = 0;
	
	/**
	 * @brief Maximum size of HID report buffer
	 */
	const static size_t BUFFER_SIZE = 1024;
	
	/**
	 * @brief Get the current HID report buffer
	 * 
	 * @return const std::array<uint8_t, BUFFER_SIZE>& Current report data
	 */
	const std::array<uint8_t, BUFFER_SIZE>& buffer() const { return buffer_; }
	
	/**
	 * @brief Get the previous HID report buffer
	 * 
	 * Useful for detecting changes between reports.
	 * 
	 * @return const std::array<uint8_t, BUFFER_SIZE>& Previous report data
	 */
	const std::array<uint8_t, BUFFER_SIZE>& prev_buffer() const
	{
		return prev_buffer_;
	}

private:
	endpoint_id ep_interrupt_in_;   ///< Interrupt IN endpoint (device to host)
	endpoint_id ep_interrupt_out_;  ///< Interrupt OUT endpoint (host to device)
	const int interface_index_;     ///< HID interface index
	int in_packet_size_;            ///< Maximum packet size for IN transfers
	int initialized_stage_{ 0 };    ///< Initialization progress tracker

	std::array<uint8_t, BUFFER_SIZE> buffer_{};      ///< Current HID report buffer
	std::array<uint8_t, BUFFER_SIZE> prev_buffer_{};  ///< Previous HID report buffer
};
} // namespace kernel::hw::usb