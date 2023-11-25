/**
 * @file hardware/usb/xhci/device.hpp
 *
 *
 */

#pragma once

#include "../array_map.hpp"
#include "../class_driver/base.hpp"
#include "../device.hpp"
#include "../setup_stage_data.hpp"
#include "context.hpp"
#include "registers.hpp"
#include "ring.hpp"

#include <array>
#include <cstdint>

namespace usb::xhci
{
class device : public usb::device
{
public:
	enum class slot_state {
		INVALID,
		BLANK,
		SLOT_ASSIGNING,
		SLOT_ASSIGNED,
	};

	device(uint8_t slot_id, doorbell_register* doorbell_register);

	void initialize();

	device_context* context() { return &ctx_; }
	input_context* input_context() { return &input_ctx_; }

	slot_state slot_state() { return state_; }
	uint8_t slot_id() { return slot_id_; }

	void select_for_slot_assignment();
	ring* alloc_transfer_ring(device_context_index index, size_t buf_size);

	void control_in(endpoint_id ep_id,
					setup_stage_data setup_data,
					void* buf,
					int len,
					class_driver* driver) override;
	void control_out(endpoint_id ep_id,
					 setup_stage_data setup_data,
					 const void* buf,
					 int len,
					 class_driver* driver) override;
	void interrupt_in(endpoint_id ep_id, void* buf, int len) override;
	void interrupt_out(endpoint_id ep_id, const void* buf, int len) override;

	void on_transfer_event_received(const transfer_event_trb& trb);

private:
	alignas(64) struct device_context ctx_;
	alignas(64) struct input_context input_ctx_;

	const uint8_t slot_id_;
	doorbell_register* const doorbell_register_;

	enum slot_state state_;
	std::array<ring*, 31> transfer_rings_;
	array_map<const void*, const setup_stage_trb*, 16> setup_stages_{};
};

} // namespace usb::xhci