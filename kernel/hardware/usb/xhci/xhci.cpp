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

	klogger->info("xHCI initialized successfully.");
}
} // namespace usb::xhci