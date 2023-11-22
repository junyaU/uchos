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

void device::control_out(endpoint_id ep_id,
						 setup_stage_data setup_data,
						 const void* buf,
						 int len,
						 class_driver* driver)
{
	usb::device::control_out(ep_id, setup_data, buf, len, driver);

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
	status_trb.bits.direction = true;

	if (buf != nullptr) {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						setup_data, setup_stage_trb::OUT_DATA_STAGE)));
		auto data_trb = make_data_stage_trb(buf, len, false);
		data_trb.bits.interrupt_on_completion = true;
		auto* data_trb_position = transfer_ring->push(data_trb);
		transfer_ring->push(status_trb);

		setup_stages_.put(data_trb_position, setup_trb_position);
	} else {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						setup_data, setup_stage_trb::NO_DATA_STAGE)));
		status_trb.bits.interrupt_on_completion = true;
		auto* status_trb_position = transfer_ring->push(status_trb);

		setup_stages_.put(status_trb_position, setup_trb_position);
	}

	doorbell_register_->ring(dc_index.value);
}

void device::interrupt_in(endpoint_id ep_id, void* buf, int len)
{
	usb::device::interrupt_in(ep_id, buf, len);

	const device_context_index dc_index{ ep_id };
	ring* transfer_ring = transfer_rings_[dc_index.value - 1];
	if (transfer_ring == nullptr) {
		klogger->error("transfer ring is not allocated");
		return;
	}

	normal_trb trb{};
	trb.set_pointer(buf);
	trb.bits.trb_transfer_length = len;
	trb.bits.interrupt_on_short_packet = true;
	trb.bits.interrupt_on_completion = true;

	transfer_ring->push(trb);
	doorbell_register_->ring(dc_index.value);
}

void device::interrupt_out(endpoint_id ep_id, const void* buf, int len)
{
	usb::device::interrupt_out(ep_id, buf, len);

	klogger->printf("interrupt_out: ep_id: %d, buf: %p, len: %d\n", ep_id.number(),
					buf, len);
}

void device::on_transfer_event_received(const transfer_event_trb& event_trb)
{
	const auto residual_length = event_trb.bits.trb_transfer_length;

	bool is_success = event_trb.bits.completion_code == 1 ||
					  event_trb.bits.completion_code == 13;
	if (!is_success) {
		klogger->printf("transfer failed: completion code: %d\n",
						event_trb.bits.completion_code);
		return;
	}

	trb* issued_trb = event_trb.pointer();

	// interrupt on completion
	if (auto normal = trb_dynamic_cast<normal_trb>(issued_trb)) {
		const auto transfer_length =
				normal->bits.trb_transfer_length - residual_length;
		return this->on_interrupt_completed(event_trb.endpoint_id(),
											normal->pointer(), transfer_length);
	}

	auto opt_setup_stage_trb = setup_stages_.get(issued_trb);
	if (!opt_setup_stage_trb) {
		klogger->error("setup stage trb not found");
		return;
	}

	setup_stages_.remove(issued_trb);

	auto setup_trb = opt_setup_stage_trb.value();
	setup_stage_data setup_data{};
	setup_data.request_type.data = setup_trb->bits.request_type;
	setup_data.request = setup_trb->bits.request;
	setup_data.value = setup_trb->bits.value;
	setup_data.index = setup_trb->bits.index;
	setup_data.length = setup_data.length;

	void* data_stage_buf = { nullptr };
	int transfer_length = { 0 };
	if (auto data_stage = trb_dynamic_cast<data_stage_trb>(issued_trb)) {
		data_stage_buf = data_stage->pointer();
		transfer_length = data_stage->bits.trb_transfer_length - residual_length;
	} else if (auto status_stage = trb_dynamic_cast<status_stage_trb>(issued_trb)) {
		// status stage
	} else {
		klogger->error("invalid trb type");
		return;
	}

	return this->on_control_completed(event_trb.endpoint_id(), setup_data,
									  data_stage_buf, transfer_length);
}

} // namespace usb::xhci