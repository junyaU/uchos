#include "device.hpp"
#include "../../../graphics/kernel_logger.hpp"
#include "../memory.hpp"
#include "ring.hpp"

namespace usb::xhci
{
device::device(uint8_t slot_id, doorbell_register* doorbell_register)
	: slot_id_{ slot_id }, doorbell_register_{ doorbell_register }
{
}

void device::initialize() { state_ = slot_state::BLANK; }

void device::select_for_slot_assignment() { state_ = slot_state::SLOT_ASSIGNING; }

ring* device::alloc_transfer_ring(device_context_index index, size_t buf_size)
{
	int i = index.value - 1;
	auto* transfer_ring = alloc_array<ring>(1, 64, 4096);
	if (transfer_ring != nullptr) {
		transfer_ring->initialize(buf_size);
	}

	transfer_rings_[i] = transfer_ring;

	return transfer_ring;
}

void device::control_in(endpoint_id ep_id,
						setup_stage_data setup_data,
						void* buf,
						int len,
						class_driver* driver)
{
	usb::device::control_in(ep_id, setup_data, buf, len, driver);

	if (ep_id.number() < 0 || ep_id.number() > 15) {
		klogger->error("invalid endpoint id");
		return;
	}

	const device_context_index dc_index{ ep_id };
	ring* transfer_ring = transfer_rings_[dc_index.value - 1];
	if (transfer_ring == nullptr) {
		klogger->error("transfer ring is not allocated");
		return;
	}

	auto status_trb = status_stage_trb{};

	if (buf != nullptr) {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						setup_data, setup_stage_trb::IN_DATA_STAGE)));
		auto data_trb = make_data_stage_trb(buf, len, true);
		data_trb.bits.interrupt_on_completion = true;
		auto* data_trb_position = transfer_ring->push(data_trb);

		transfer_ring->push(status_trb);
		setup_stages_.put(data_trb_position, setup_trb_position);
	} else {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						setup_data, setup_stage_trb::NO_DATA_STAGE)));
		status_trb.bits.direction = true;
		status_trb.bits.interrupt_on_completion = true;
		auto* status_trb_position = transfer_ring->push(status_trb);

		setup_stages_.put(status_trb_position, setup_trb_position);
	}

	doorbell_register_->ring(dc_index.value);
}

} // namespace usb::xhci