/**
 * @file hardware/usb/device.hpp
 *
 * @brief USB device
 *
 * This file defines the 'device' class, which serves as an abstract base class for
 * representing a USB device within a USB system. This class provides the fundamental
 * interface and functionalities necessary to interact with and manage a USB device.
 *
 */

#pragma once

#include "array_map.hpp"
#include "class_driver/base.hpp"
#include "endpoint.hpp"
#include "setup_stage_data.hpp"

#include <array>

namespace usb
{
class class_driver;

struct control_transfer_data {
	const endpoint_id& ep_id;
	setup_stage_data setup_data;
	void* buf;
	int len;
	class_driver* driver;
};

struct interrupt_transfer_data {
	const endpoint_id& ep_id;
	void* buf;
	int len;
};

class device
{
public:
	virtual ~device();
	virtual void control_in(const control_transfer_data& data) = 0;
	virtual void control_out(const control_transfer_data& data) = 0;
	virtual void interrupt_in(const interrupt_transfer_data& data) = 0;
	virtual void interrupt_out(const interrupt_transfer_data& data) = 0;

	void start_initialize();
	bool is_initialized() const { return is_initialized_; }

	endpoint_config* endpoint_configs() { return endpoint_configs_.data(); }
	int num_endpoint_configs() const { return num_endpoint_configs_; }
	void on_endpoints_configured();

	uint8_t* buffer() { return buffer_.data(); }

protected:
	void on_control_completed(const control_transfer_data& data);
	void on_interrupt_completed(const interrupt_transfer_data& data);

private:
	std::array<class_driver*, 16> class_drivers_{};
	std::array<uint8_t, 256> buffer_{};

	uint8_t num_configurations_;
	uint8_t config_index_;

	void on_device_descriptor_received(const uint8_t* buf, int len);
	void on_configuration_descriptor_received(const uint8_t* buf, int len);
	void on_set_configuration_completed(uint8_t config_value);

	bool is_initialized_{ false };
	int initialize_stage_{ 0 };
	std::array<endpoint_config, 16> endpoint_configs_;
	int num_endpoint_configs_;

	void initialize_stage1(const uint8_t* buf, int len);
	void initialize_stage2(const uint8_t* buf, int len);
	void initialize_stage3(const uint8_t* buf, int len);

	array_map<setup_stage_data, class_driver*, 4> event_waiters_{};
};

void get_descriptor(device& dev,
					const endpoint_id& ep_id,
					uint8_t desc_type,
					uint8_t desc_index,
					void* buf,
					int len,
					bool debug = false);
void set_configuration(device& dev,
					   const endpoint_id& ep_id,
					   uint8_t config_value,
					   bool debug = false);
} // namespace usb