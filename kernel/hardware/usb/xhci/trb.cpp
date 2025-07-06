#include "trb.hpp"
#include "../setup_stage_data.hpp"

namespace kernel::hw::usb::xhci
{
setup_stage_trb make_setup_stage_trb(setup_stage_data setup_data, int transfer_type)
{
	setup_stage_trb trb{};
	trb.bits.request_type = setup_data.request_type.data;
	trb.bits.request = setup_data.request;
	trb.bits.value = setup_data.value;
	trb.bits.index = setup_data.index;
	trb.bits.length = setup_data.length;
	trb.bits.transfer_type = transfer_type;

	return trb;
}

data_stage_trb make_data_stage_trb(const void* buf, int len, bool dir_in)
{
	data_stage_trb trb{};
	trb.set_pointer(buf);
	trb.bits.trb_transfer_length = len;
	trb.bits.td_size = 0;
	trb.bits.direction = dir_in;

	return trb;
}

} // namespace kernel::hw::usb::xhci