/**
 * @file hardware/usb/class_driver/base.hpp
 *
 * Base class for USB class drivers.
 * This class provides the interface and common functionalities for
 * different types of USB class drivers. It's designed to be subclassed
 * for specific device protocols.
 *
 */

#pragma once

#include "../descriptor.hpp"
#include "../endpoint.hpp"
#include "../setup_stage_data.hpp"

namespace kernel::hw::usb
{
class device;

class class_driver
{
public:
	class_driver(device* dev);
	virtual ~class_driver();

	virtual void initialize() = 0;
	virtual void set_endpoint(const endpoint_config& config) = 0;
	virtual void on_endpoints_configured() = 0;
	virtual void on_control_completed(endpoint_id ep_id,
									  setup_stage_data setup_data,
									  void* buf,
									  int len) = 0;
	virtual void on_interrupt_completed(endpoint_id ep_id, void* buf, int len) = 0;

	device* parent_device() { return device_; }

private:
	device* device_;
};

class_driver* new_class_driver(device* dev, const interface_descriptor& if_desc);

} // namespace kernel::hw::usb
