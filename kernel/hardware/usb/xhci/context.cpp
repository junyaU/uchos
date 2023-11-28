#include "context.hpp"

namespace usb::xhci
{
void initialize_slot_context(slot_context& ctx, uint8_t port_id, int port_speed)
{
	ctx.bits.route_string = 0;
	ctx.bits.root_hub_port_num = port_id;
	ctx.bits.context_entries = 1;
	ctx.bits.speed = port_speed;
}

void initialize_endpoint0_context(endpoint_context& ctx,
								  trb* transfer_ring_buffer,
								  int max_packet_size)
{
	ctx.bits.ep_type = 4;
	ctx.bits.max_packet_size = max_packet_size;
	ctx.bits.max_burst_size = 0;
	ctx.set_transfer_ring_buffer(transfer_ring_buffer);
	ctx.bits.dequeue_cycle_state = 1;
	ctx.bits.interval = 0;
	ctx.bits.max_primary_streams = 0;
	ctx.bits.mult = 0;
	ctx.bits.error_count = 3;
}
} // namespace usb::xhci