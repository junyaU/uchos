#include "device.hpp"
#include "../../graphics/terminal.hpp"
#include "class_driver/base.hpp"
#include "descriptor.hpp"

namespace usb
{
device::~device() {}

void device::control_in(const control_transfer_data& data)
{
	if (data.driver != nullptr) {
		event_waiters_.put(data.setup_data, data.driver);
	}
}

void device::control_out(const control_transfer_data& data)
{
	if (data.driver != nullptr) {
		event_waiters_.put(data.setup_data, data.driver);
	}
}

void device::interrupt_in(const interrupt_transfer_data& data) {}

void device::interrupt_out(const interrupt_transfer_data& data) {}

void device::start_initialize()
{
	is_initialized_ = false;
	initialize_stage_ = 1;

	get_descriptor(*this, DEFAULT_CONTROL_PIPE_ID, device_descriptor::TYPE, 0,
				   buffer_.data(), buffer_.size(), true);
}

void device::on_endpoints_configured()
{
	for (auto* class_driver : class_drivers_) {
		if (class_driver != nullptr) {
			class_driver->on_endpoints_configured();
		}
	}
}

void device::on_control_completed(const control_transfer_data& data)
{
	if (is_initialized_) {
		if (auto w = event_waiters_.get(data.setup_data)) {
			return w.value()->on_control_completed(data.ep_id, data.setup_data,
												   data.buf, data.len);
		}
		return;
	}

	const uint8_t* buf8 = reinterpret_cast<const uint8_t*>(data.buf);
	switch (initialize_stage_) {
		case 1:
			if (data.setup_data.request == request::GET_DESCRIPTOR &&
				(descriptor_dynamic_cast<device_descriptor>(buf8) != nullptr)) {
				return initialize_stage1(buf8, data.len);
			}
		case 2:
			if (data.setup_data.request == request::GET_DESCRIPTOR &&
				(descriptor_dynamic_cast<configuration_descriptor>(buf8) !=
				 nullptr)) {
				return initialize_stage2(buf8, data.len);
			}
		case 3:
			if (data.setup_data.request == request::SET_CONFIGURATION) {
				return initialize_stage3(buf8, data.len);
			}
		default:
			printk(KERN_ERROR, "Device: on_control_completed: invalid stage");
			return;
	}
}

void device::on_interrupt_completed(const interrupt_transfer_data& data)
{
	if (auto* w = class_drivers_[data.ep_id.number()]; w != nullptr) {
		return w->on_interrupt_completed(data.ep_id, data.buf, data.len);
	}

	printk(KERN_ERROR, "Device: on_interrupt_completed: invalid endpoint");
}

void device::initialize_stage1(const uint8_t* buf, int len)
{
	const auto* device_desc = descriptor_dynamic_cast<device_descriptor>(buf);
	num_configurations_ = device_desc->num_configurations;
	config_index_ = 0;
	initialize_stage_ = 2;

	get_descriptor(*this, DEFAULT_CONTROL_PIPE_ID, configuration_descriptor::TYPE,
				   config_index_, buffer_.data(), buffer_.size(), true);
}

void device::initialize_stage2(const uint8_t* buf, int len)
{
	const auto* conf_desc = descriptor_dynamic_cast<configuration_descriptor>(buf);
	if (conf_desc == nullptr) {
		return;
	}

	configuration_descriptor_iterator config_it{ buf, len };
	class_driver* class_driver = nullptr;

	while (const auto* if_desc = config_it.next<interface_descriptor>()) {
		class_driver = new_class_driver(this, *if_desc);
		if (class_driver == nullptr) {
			continue;
		}

		num_endpoint_configs_ = 0;

		while (num_endpoint_configs_ < if_desc->num_endpoints) {
			const auto* desc = config_it.next();
			if (const auto* ep_desc =
						descriptor_dynamic_cast<endpoint_descriptor>(desc);
				ep_desc != nullptr) {
				auto conf = make_endpoint_config(*ep_desc);

				endpoint_configs_[num_endpoint_configs_++] = conf;
				class_drivers_[conf.id.number()] = class_driver;
			}
		}

		break;
	}

	if (class_driver == nullptr) {
		return;
	}

	initialize_stage_ = 3;
	return set_configuration(*this, DEFAULT_CONTROL_PIPE_ID,
							 conf_desc->configuration_value, true);
}

void device::initialize_stage3(const uint8_t* buf, int len)
{
	for (int i = 0; i < num_endpoint_configs_; i++) {
		class_drivers_[endpoint_configs_[i].id.number()]->set_endpoint(
				endpoint_configs_[i]);
	}

	initialize_stage_ = 4;
	is_initialized_ = true;
}

void get_descriptor(device& dev,
					const endpoint_id& ep_id,
					uint8_t desc_type,
					uint8_t desc_index,
					void* buf,
					int len,
					bool debug)
{
	setup_stage_data setup_data{};
	setup_data.request_type.bits.direction = request_type::IN;
	setup_data.request_type.bits.type = request_type::STANDARD;
	setup_data.request_type.bits.recipient = request_type::DEVICE;
	setup_data.request = request::GET_DESCRIPTOR;
	setup_data.value = (static_cast<uint16_t>(desc_type) << 8) | desc_index;
	setup_data.index = 0;
	setup_data.length = len;

	dev.control_in({ ep_id, setup_data, buf, len, nullptr });
}

void set_configuration(device& dev,
					   const endpoint_id& ep_id,
					   uint8_t config_value,
					   bool debug)
{
	setup_stage_data setup_data{};
	setup_data.request_type.bits.direction = request_type::OUT;
	setup_data.request_type.bits.type = request_type::STANDARD;
	setup_data.request_type.bits.recipient = request_type::DEVICE;
	setup_data.request = request::SET_CONFIGURATION;
	setup_data.value = config_value;
	setup_data.index = 0;
	setup_data.length = 0;

	return dev.control_out({ ep_id, setup_data, nullptr, 0, nullptr });
}

} // namespace usb
