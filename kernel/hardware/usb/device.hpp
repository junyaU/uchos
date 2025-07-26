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

namespace kernel::hw::usb
{
class ClassDriver;

struct ControlTransferData {
	const EndpointId& ep_id;
	SetupStageData setup_data;
	void* buf;
	int len;
	ClassDriver* driver;
};

struct InterruptTransferData {
	const EndpointId& ep_id;
	void* buf;
	int len;
};

class Device
{
public:
	virtual ~Device();
	virtual void control_in(const ControlTransferData& data) = 0;
	virtual void control_out(const ControlTransferData& data) = 0;
	virtual void interrupt_in(const InterruptTransferData& data) = 0;
	virtual void interrupt_out(const InterruptTransferData& data) = 0;

	void start_initialize();
	bool is_initialized() const { return is_initialized_; }

	EndpointConfig* EndpointConfigs() { return EndpointConfigs_.data(); }
	int num_EndpointConfigs() const { return num_EndpointConfigs_; }
	void on_endpoints_configured();

	uint8_t* buffer() { return buffer_.data(); }

protected:
	void on_control_completed(const ControlTransferData& data);
	void on_interrupt_completed(const InterruptTransferData& data);

private:
	std::array<ClassDriver*, 16> ClassDrivers_{};
	std::array<uint8_t, 256> buffer_{};

	uint8_t num_configurations_;
	uint8_t config_index_;

	void on_device_descriptor_received(const uint8_t* buf, int len);
	void on_configuration_descriptor_received(const uint8_t* buf, int len);
	void on_set_configuration_completed(uint8_t config_value);

	bool is_initialized_{ false };
	int initialize_stage_{ 0 };
	std::array<EndpointConfig, 16> EndpointConfigs_;
	int num_EndpointConfigs_;

	void initialize_stage1(const uint8_t* buf, int len);
	void initialize_stage2(const uint8_t* buf, int len);
	void initialize_stage3(const uint8_t* buf, int len);

	ArrayMap<SetupStageData, ClassDriver*, 4> event_waiters_{};
};

void get_descriptor(Device& dev,
					const EndpointId& ep_id,
					uint8_t desc_type,
					uint8_t desc_index,
					void* buf,
					int len,
					bool debug = false);
void set_configuration(Device& dev,
					   const EndpointId& ep_id,
					   uint8_t config_value,
					   bool debug = false);
} // namespace kernel::hw::usb