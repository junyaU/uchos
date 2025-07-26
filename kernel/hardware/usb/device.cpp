#include "device.hpp"
#include <cstdint>
#include "../../graphics/log.hpp"
#include "class_driver/base.hpp"
#include "descriptor.hpp"
#include "endpoint.hpp"
#include "setup_stage_data.hpp"

namespace kernel::hw::usb
{
Device::~Device() {}

void Device::control_in(const ControlTransferData& data)
{
	if (data.driver != nullptr) {
		event_waiters_.put(data.setup_data, data.driver);
	}
}

void Device::control_out(const ControlTransferData& data)
{
	if (data.driver != nullptr) {
		event_waiters_.put(data.setup_data, data.driver);
	}
}

void Device::interrupt_in(const InterruptTransferData& data) {}

void Device::interrupt_out(const InterruptTransferData& data) {}

void Device::start_initialize()
{
	is_initialized_ = false;
	initialize_stage_ = 1;

	get_descriptor(*this, DEFAULT_CONTROL_PIPE_ID, DeviceDescriptor::TYPE, 0,
				   buffer_.data(), buffer_.size(), true);
}

void Device::on_endpoints_configured()
{
	for (auto* class_driver : ClassDrivers_) {
		if (class_driver != nullptr) {
			class_driver->on_endpoints_configured();
		}
	}
}

void Device::on_control_completed(const ControlTransferData& data)
{
	if (is_initialized_) {
		if (auto w = event_waiters_.get(data.setup_data)) {
			w.value()->on_control_completed(data.ep_id, data.setup_data,
												   data.buf, data.len);
			return;
		}
		return;
	}

	const uint8_t* buf8 = reinterpret_cast<const uint8_t*>(data.buf);
	switch (initialize_stage_) {
		case 1:
			if (data.setup_data.request == request::GET_DESCRIPTOR &&
				(descriptor_dynamic_cast<DeviceDescriptor>(buf8) != nullptr)) {
				initialize_stage1(buf8, data.len);
				return;
			}
		case 2:
			if (data.setup_data.request == request::GET_DESCRIPTOR &&
				(descriptor_dynamic_cast<ConfigurationDescriptor>(buf8) !=
				 nullptr)) {
				initialize_stage2(buf8, data.len);
				return;
			}
		case 3:
			if (data.setup_data.request == request::SET_CONFIGURATION) {
				initialize_stage3(buf8, data.len);
				return;
			}
		default:
			LOG_ERROR("invalid stage");
			return;
	}
}

void Device::on_interrupt_completed(const InterruptTransferData& data)
{
	if (auto* w = ClassDrivers_[data.ep_id.number()]; w != nullptr) {
		w->on_interrupt_completed(data.ep_id, data.buf, data.len);
		return;
	}

	LOG_ERROR("invalid endpoint");
}

void Device::initialize_stage1(const uint8_t* buf, int len)
{
	const auto* device_desc = descriptor_dynamic_cast<DeviceDescriptor>(buf);
	num_configurations_ = device_desc->num_configurations;
	config_index_ = 0;
	initialize_stage_ = 2;

	get_descriptor(*this, DEFAULT_CONTROL_PIPE_ID, ConfigurationDescriptor::TYPE,
				   config_index_, buffer_.data(), buffer_.size(), true);
}

void Device::initialize_stage2(const uint8_t* buf, int len)
{
	const auto* conf_desc = descriptor_dynamic_cast<ConfigurationDescriptor>(buf);
	if (conf_desc == nullptr) {
		return;
	}

	ConfigurationDescriptorIterator config_it{ buf, len };
	ClassDriver* class_driver = nullptr;

	while (const auto* if_desc = config_it.next<InterfaceDescriptor>()) {
		class_driver = new_class_driver(this, *if_desc);
		if (class_driver == nullptr) {
			continue;
		}

		num_EndpointConfigs_ = 0;

		while (num_EndpointConfigs_ < if_desc->num_endpoints) {
			const auto* desc = config_it.next();
			if (const auto* ep_desc =
						descriptor_dynamic_cast<EndpointDescriptor>(desc);
				ep_desc != nullptr) {
				auto conf = make_endpoint_config(*ep_desc);

				EndpointConfigs_[num_EndpointConfigs_++] = conf;
				ClassDrivers_[conf.id.number()] = class_driver;
			}
		}

		break;
	}

	if (class_driver == nullptr) {
		return;
	}

	initialize_stage_ = 3;
	set_configuration(*this, DEFAULT_CONTROL_PIPE_ID,
							 conf_desc->configuration_value, true);
}

void Device::initialize_stage3(const uint8_t* buf, int len)
{
	for (int i = 0; i < num_EndpointConfigs_; i++) {
		ClassDrivers_[EndpointConfigs_[i].id.number()]->set_endpoint(
				EndpointConfigs_[i]);
	}

	initialize_stage_ = 4;
	is_initialized_ = true;
}

void get_descriptor(Device& dev,
					const EndpointId& ep_id,
					uint8_t desc_type,
					uint8_t desc_index,
					void* buf,
					int len,
					bool debug)
{
	SetupStageData setup_data{};
	setup_data.request_type.bits.direction = request_type::IN;
	setup_data.request_type.bits.type = request_type::STANDARD;
	setup_data.request_type.bits.recipient = request_type::DEVICE;
	setup_data.request = request::GET_DESCRIPTOR;
	setup_data.value = (static_cast<uint16_t>(desc_type) << 8) | desc_index;
	setup_data.index = 0;
	setup_data.length = len;

	dev.control_in({ ep_id, setup_data, buf, len, nullptr });
}

void set_configuration(Device& dev,
					   const EndpointId& ep_id,
					   uint8_t config_value,
					   bool debug)
{
	SetupStageData setup_data{};
	setup_data.request_type.bits.direction = request_type::OUT;
	setup_data.request_type.bits.type = request_type::STANDARD;
	setup_data.request_type.bits.recipient = request_type::DEVICE;
	setup_data.request = request::SET_CONFIGURATION;
	setup_data.value = config_value;
	setup_data.index = 0;
	setup_data.length = 0;

	dev.control_out({ ep_id, setup_data, nullptr, 0, nullptr });
}

} // namespace kernel::hw::usb
