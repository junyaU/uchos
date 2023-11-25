#include "registers.hpp"

namespace
{
template<class ptr, class disp>
ptr add_or_null(ptr p, disp d)
{
	return d == 0 ? nullptr : p + d;
}
} // namespace

namespace usb::xhci
{
extended_register_list::iterator& extended_register_list::iterator::operator++()
{
	if (reg_ != nullptr) {
		reg_ = add_or_null(reg_, reg_->read().bits.next_capability_pointer);
		static_assert(sizeof(*reg_) == 4);
	}
	return *this;
}

extended_register_list::extended_register_list(uint64_t mmio_base,
											   usb::xhci::hcc_params1_register hccp)
	: begin_(add_or_null(reinterpret_cast<value_type*>(mmio_base),
						 hccp.bits.xhci_extended_capabilities_pointer))
{
}
} // namespace usb::xhci