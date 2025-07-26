/**
 * @file hardware/usb/xhci/device.hpp
 *
 *
 */

#pragma once

#include "../array_map.hpp"
#include "../class_driver/base.hpp"
#include "../device.hpp"
#include "context.hpp"
#include "registers.hpp"
#include "ring.hpp"

#include <array>
#include <cstdint>

namespace kernel::hw::usb::xhci
{
class Device : public kernel::hw::usb::Device
{
public:
	enum class SlotState {
		INVALID,
		BLANK,
		SLOT_ASSIGNING,
		SLOT_ASSIGNED,
	};

	Device(uint8_t slot_id, DoorbellRegister* doorbell_register);

	void initialize();

	DeviceContext* context() { return &ctx_; }
	InputContext* input_context() { return &input_ctx_; }

	SlotState slot_state() { return state_; }
	uint8_t slot_id() { return slot_id_; }

	void select_for_slot_assignment();
	Ring* alloc_transfer_ring(DeviceContextIndex index, size_t buf_size);

	void control_in(const ControlTransferData& data) override;
	void control_out(const ControlTransferData& data) override;
	void interrupt_in(const InterruptTransferData& data) override;
	void interrupt_out(const InterruptTransferData& data) override;

	void on_transfer_event_received(const transfer_event_trb& trb);

private:
	alignas(64) struct DeviceContext ctx_;
	alignas(64) struct InputContext input_ctx_;

	const uint8_t slot_id_;
	DoorbellRegister* const doorbell_register_;

	enum SlotState state_;
	std::array<Ring*, 31> transfer_rings_;
	ArrayMap<const void*, const setup_stage_trb*, 16> setup_stages_{};
};

} // namespace kernel::hw::usb::xhci