#include "device.hpp"
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include "graphics/log.hpp"
#include "ring.hpp"
#include "registers.hpp"
#include "trb.hpp"
#include "context.hpp"
#include "../device.hpp"

namespace kernel::hw::usb::xhci
{
Device::Device(uint8_t slot_id, DoorbellRegister* doorbell_register)
	: slot_id_{ slot_id }, doorbell_register_{ doorbell_register }
{
}

void Device::initialize() { state_ = slot_state::BLANK; }

void Device::select_for_slot_assignment() { state_ = slot_state::SLOT_ASSIGNING; }

Ring* Device::alloc_transfer_ring(DeviceContextIndex index, size_t buf_size)
{
	const int i = index.value - 1;
	auto* transfer_ring = reinterpret_cast<Ring*>(malloc(64));
	if (transfer_ring != nullptr) {
		transfer_ring->initialize(buf_size);
	}

	transfer_rings_[i] = transfer_ring;

	return transfer_ring;
}

void Device::control_in(const ControlTransferData& data)
{
	kernel::hw::usb::Device::control_in(data);

	if (data.ep_id.number() < 0 || data.ep_id.number() > 15) {
		LOG_ERROR("invalid endpoint id");
		return;
	}

	const DeviceContextIndex dc_index{ data.ep_id };
	Ring* transfer_ring = transfer_rings_[dc_index.value - 1];
	if (transfer_ring == nullptr) {
		LOG_ERROR("transfer ring is not allocated");
		return;
	}

	auto status_trb = status_stage_trb{};

	if (data.buf != nullptr) {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						data.setup_data, setup_stage_trb::IN_DATA_STAGE)));
		auto data_trb = make_data_stage_trb(data.buf, data.len, true);
		data_trb.bits.interrupt_on_completion = true;
		auto* data_trb_position = transfer_ring->push(data_trb);

		transfer_ring->push(status_trb);
		setup_stages_.put(data_trb_position, setup_trb_position);
	} else {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						data.setup_data, setup_stage_trb::NO_DATA_STAGE)));
		status_trb.bits.direction = true;
		status_trb.bits.interrupt_on_completion = true;
		auto* status_trb_position = transfer_ring->push(status_trb);

		setup_stages_.put(status_trb_position, setup_trb_position);
	}

	doorbell_register_->ring(dc_index.value);
}

void Device::control_out(const ControlTransferData& data)
{
	kernel::hw::usb::Device::control_out(data);

	if (data.ep_id.number() < 0 || data.ep_id.number() > 15) {
		LOG_ERROR("invalid endpoint id");
		return;
	}

	const DeviceContextIndex dc_index{ data.ep_id };
	Ring* transfer_ring = transfer_rings_[dc_index.value - 1];
	if (transfer_ring == nullptr) {
		LOG_ERROR("transfer ring is not allocated");
		return;
	}

	auto status_trb = status_stage_trb{};
	status_trb.bits.direction = true;

	if (data.buf != nullptr) {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						data.setup_data, setup_stage_trb::OUT_DATA_STAGE)));
		auto data_trb = make_data_stage_trb(data.buf, data.len, false);
		data_trb.bits.interrupt_on_completion = true;
		auto* data_trb_position = transfer_ring->push(data_trb);
		transfer_ring->push(status_trb);

		setup_stages_.put(data_trb_position, setup_trb_position);
	} else {
		auto* setup_trb_position = trb_dynamic_cast<setup_stage_trb>(
				transfer_ring->push(make_setup_stage_trb(
						data.setup_data, setup_stage_trb::NO_DATA_STAGE)));
		status_trb.bits.interrupt_on_completion = true;
		auto* status_trb_position = transfer_ring->push(status_trb);

		setup_stages_.put(status_trb_position, setup_trb_position);
	}

	doorbell_register_->ring(dc_index.value);
}

void Device::interrupt_in(const InterruptTransferData& data)
{
	kernel::hw::usb::Device::interrupt_in(data);

	const DeviceContextIndex dc_index{ data.ep_id };
	Ring* transfer_ring = transfer_rings_[dc_index.value - 1];
	if (transfer_ring == nullptr) {
		LOG_ERROR("transfer ring is not allocated");
		return;
	}

	normal_trb trb{};
	trb.set_pointer(data.buf);
	trb.bits.trb_transfer_length = data.len;
	trb.bits.interrupt_on_short_packet = true;
	trb.bits.interrupt_on_completion = true;

	transfer_ring->push(trb);
	doorbell_register_->ring(dc_index.value);
}

void Device::interrupt_out(const InterruptTransferData& data)
{
	kernel::hw::usb::Device::interrupt_out(data);

	LOG_DEBUG("interrupt_out: ep_id: %d, buf: %p, len: %d\n", data.ep_id.number(),
			  data.buf, data.len);
}

void Device::on_transfer_event_received(const transfer_event_trb& event_trb)
{
	const auto residual_length = event_trb.bits.trb_transfer_length;

	const bool is_success = event_trb.bits.completion_code == 1 ||
							event_trb.bits.completion_code == 13;
	if (!is_success) {
		LOG_DEBUG("transfer failed: completion code: %d\n",
				  event_trb.bits.completion_code);
		return;
	}

	trb* issued_trb = event_trb.pointer();

	// interrupt on completion
	if (auto* normal = trb_dynamic_cast<normal_trb>(issued_trb)) {
		const auto transfer_length =
				normal->bits.trb_transfer_length - residual_length;
		this->on_interrupt_completed({ event_trb.EndpointId(),
											  normal->pointer(),
											  static_cast<int>(transfer_length) });
		return;
	}

	auto opt_setup_stage_trb = setup_stages_.get(issued_trb);
	if (!opt_setup_stage_trb) {
		LOG_ERROR("setup stage trb not found");
		return;
	}

	setup_stages_.remove(issued_trb);

	const auto* setup_trb = opt_setup_stage_trb.value();
	SetupStageData setup_data{};
	setup_data.request_type.data = setup_trb->bits.request_type;
	setup_data.request = setup_trb->bits.request;
	setup_data.value = setup_trb->bits.value;
	setup_data.index = setup_trb->bits.index;
	setup_data.length = setup_data.length;

	void* data_stage_buf = { nullptr };
	int transfer_length = { 0 };
	if (auto* data_stage = trb_dynamic_cast<data_stage_trb>(issued_trb)) {
		data_stage_buf = data_stage->pointer();
		transfer_length = data_stage->bits.trb_transfer_length - residual_length;
	} else if (auto* status_stage = trb_dynamic_cast<status_stage_trb>(issued_trb);
			   status_stage == nullptr) {
		LOG_ERROR("invalid trb type");
		return;
	}

	this->on_control_completed({ event_trb.EndpointId(), setup_data,
										data_stage_buf, transfer_length });
}

} // namespace kernel::hw::usb::xhci