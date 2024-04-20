#include "hardware/virtio/virtio.hpp"
#include "graphics/log.hpp"
#include "hardware/pci.hpp"
#include "interrupt/vector.hpp"
#include <libs/common/types.hpp>

error_t init_virtio_pci()
{
	pci::device* virtio_dev = nullptr;
	for (int i = 0; i < pci::num_devices; i++) {
		if (pci::devices[i].is_vertio()) {
			virtio_dev = &pci::devices[i];
			break;
		}
	}

	if (virtio_dev == nullptr) {
		printk(KERN_ERROR, "No virtio device found");
		return ERR_NO_DEVICE;
	}

	const uint8_t bsp_lapic_id = *reinterpret_cast<uint32_t*>(0xfee00020) >> 24;
	pci::configure_msi_fixed_destination(
			*virtio_dev, bsp_lapic_id, pci::msi_trigger_mode::EDGE,
			pci::msi_delivery_mode::FIXED, interrupt_vector::VIRTIO, 0);

	return OK;
}
