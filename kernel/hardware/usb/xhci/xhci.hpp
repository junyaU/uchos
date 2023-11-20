/**
 * @file hardware/usb/xhci/xhci.hpp
 *
 *
 */

#pragma once

#include <cstdint>

namespace usb::xhci
{
class controller
{
public:
	controller();

	void initialize();

private:
	const uintptr_t mmio_base_;
};
void initialize();
} // namespace usb::xhci