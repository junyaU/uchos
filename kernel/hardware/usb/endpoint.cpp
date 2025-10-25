#include "endpoint.hpp"
#include "descriptor.hpp"

namespace kernel::hw::usb
{
EndpointConfig make_endpoint_config(const EndpointDescriptor& desc)
{
	EndpointConfig config;
	config.id = EndpointId{ desc.endpoint_address.bits.number,
							desc.endpoint_address.bits.direction_in == 1 };
	config.type = static_cast<EndpointType>(desc.attributes.bits.transfer_type);
	config.max_packet_size = desc.max_packet_size;
	config.interval = desc.interval;

	return config;
}

} // namespace kernel::hw::usb