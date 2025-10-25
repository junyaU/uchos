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
class Device;

/**
 * @brief Base class for USB device class drivers
 *
 * This abstract base class defines the interface that all USB class
 * drivers must implement. Each USB device class (HID, Mass Storage,
 * Audio, etc.) should derive from this class.
 */
class ClassDriver
{
public:
	/**
	 * @brief Construct a new class driver
	 *
	 * @param dev Parent USB device this driver manages
	 */
	ClassDriver(Device* dev);

	/**
	 * @brief Virtual destructor
	 */
	virtual ~ClassDriver();

	/**
	 * @brief Initialize the class driver
	 *
	 * Called after the driver is created to perform any necessary
	 * initialization, such as sending class-specific control requests.
	 */
	virtual void initialize() = 0;

	/**
	 * @brief Configure an endpoint for this class
	 *
	 * @param config Endpoint configuration from the device descriptor
	 */
	virtual void set_endpoint(const EndpointConfig& config) = 0;

	/**
	 * @brief Called when all endpoints have been configured
	 *
	 * This is called after all endpoints from the interface descriptor
	 * have been processed and configured.
	 */
	virtual void on_endpoints_configured() = 0;

	/**
	 * @brief Handle completion of a control transfer
	 *
	 * @param ep_id Endpoint ID (usually 0 for control)
	 * @param setup_data Setup packet data
	 * @param buf Buffer containing transfer data
	 * @param len Length of data transferred
	 */
	virtual void on_control_completed(EndpointId ep_id,
									  SetupStageData setup_data,
									  void* buf,
									  int len) = 0;

	/**
	 * @brief Handle completion of an interrupt transfer
	 *
	 * @param ep_id Endpoint ID that completed
	 * @param buf Buffer containing transfer data
	 * @param len Length of data transferred
	 */
	virtual void on_interrupt_completed(EndpointId ep_id, void* buf, int len) = 0;

	/**
	 * @brief Get the parent USB device
	 *
	 * @return device* Pointer to the parent device
	 */
	Device* parent_device() { return device_; }

private:
	Device* device_; ///< Parent USB device
};

/**
 * @brief Factory function to create appropriate class driver
 *
 * Creates a new class driver based on the interface descriptor's
 * class/subclass/protocol values.
 *
 * @param dev Parent USB device
 * @param if_desc Interface descriptor describing the device class
 * @return class_driver* New class driver instance, or nullptr if unsupported
 */
ClassDriver* new_class_driver(Device* dev, const InterfaceDescriptor& if_desc);

} // namespace kernel::hw::usb
