#include "xhci.hpp"
#include "../../../graphics/kernel_logger.hpp"
#include "../../pci.hpp"

namespace usb::xhci
{
void initialize()
{
	klogger->info("Initializing xHCI...");

	pci::device* xhc_dev = nullptr;
	for (int i = 0; i < pci::num_devices; i++) {
		if (pci::devices[i].is_xhci()) {
			xhc_dev = &pci::devices[i];

			if (xhc_dev->is_intel()) {
				break;
			}
		}
	}

	if (xhc_dev == nullptr) {
		klogger->error("xHCI device not found.");
		return;
	}

	// TODO: MSI (Message Signaled Interrupts) support

	const uint64_t bar = pci::read_base_address_register(*xhc_dev, 0);
	const uint64_t xhc_mmio_base = bar & ~static_cast<uint64_t>(0xf);

	klogger->info("xHCI initialized successfully.");
}
} // namespace usb::xhci