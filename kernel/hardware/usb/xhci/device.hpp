#pragma once

#include "context.hpp"
#include "registers.hpp"

#include <cstdint>

namespace usb::xhci
{
class device
{
public:
	enum class slot_state {
		INVALID,
		BLANK,
		SLOT_ASSIGNING,
		SLOT_ASSIGNED,
	};

private:
	alignas(64) struct device_context ctx_;
	alignas(64) struct input_control_context input_ctx_;

	const uint8_t slot_id_;
	doorbell_offset_register* const doorbell_register_;

	enum slot_state state_;

};

} // namespace usb::xhci