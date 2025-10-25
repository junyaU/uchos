#include "registers.hpp"
#include <cstdint>

namespace
{
template<class ptr, class disp>
ptr add_or_null(ptr p, disp d)
{
	return d == 0 ? nullptr : p + d;
}
} // namespace

namespace kernel::hw::usb::xhci
{
ExtendedRegisterList::iterator& ExtendedRegisterList::iterator::operator++()
{
	if (reg_ != nullptr) {
		reg_ = add_or_null(reg_, reg_->read().bits.next_capability_pointer);
		static_assert(sizeof(*reg_) == 4);
	}
	return *this;
}

ExtendedRegisterList::ExtendedRegisterList(
		uint64_t mmio_base,
		kernel::hw::usb::xhci::hcc_params1_register hccp)
	: begin_(add_or_null(reinterpret_cast<value_type*>(mmio_base),
						 hccp.bits.xhci_extended_capabilities_pointer))
{
}
} // namespace kernel::hw::usb::xhci