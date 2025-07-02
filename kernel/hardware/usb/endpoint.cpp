#include "endpoint.hpp"
#include "descriptor.hpp"

namespace kernel::hw::usb
{
endpoint_config make_endpoint_config(const endpoint_descriptor& desc)
{
	endpoint_config config;
	config.id = endpoint_id{ desc.endpoint_address.bits.number,
							 desc.endpoint_address.bits.direction_in == 1 };
	config.type = static_cast<endpoint_type>(desc.attributes.bits.transfer_type);
	config.max_packet_size = desc.max_packet_size;
	config.interval = desc.interval;
	
	return config;
}

} // namespace kernel::hw::usb