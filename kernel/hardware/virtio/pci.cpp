#include "hardware/virtio/pci.hpp"
#include "bit_utils.hpp"
#include "graphics/log.hpp"
#include "hardware/pci.hpp"
#include "hardware/virtio/blk.hpp"
#include "hardware/virtio/virtio.hpp"
#include "interrupt/vector.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

size_t find_virtio_pci_cap(pci::device& virtio_dev, virtio_pci_cap** caps)
{
	uint8_t cap_id, cap_next;
	uint32_t cap_addr = pci::get_capability_pointer(virtio_dev);
	virtio_pci_cap* prev_cap = nullptr;
	size_t num_caps = 0;
	*caps = nullptr;

	while (cap_addr != 0) {
		auto header = pci::read_capability_header(virtio_dev, cap_addr);
		cap_id = header.bits.cap_id;
		cap_next = header.bits.next_ptr;

		if (cap_id == pci::CAP_VIRTIO) {
			void* addr = kmalloc(sizeof(virtio_pci_cap), KMALLOC_ZEROED);
			virtio_pci_cap* cap = new (addr) virtio_pci_cap;
			cap->first_dword.data = pci::read_conf_reg(virtio_dev, cap_addr);
			cap->second_dword.data = pci::read_conf_reg(virtio_dev, cap_addr + 4);
			cap->offset = pci::read_conf_reg(virtio_dev, cap_addr + 8);
			cap->length = pci::read_conf_reg(virtio_dev, cap_addr + 12);

			if (prev_cap != nullptr) {
				prev_cap->next = cap;
			} else {
				*caps = cap;
			}

			prev_cap = cap;
			++num_caps;
		}

		cap_addr = cap_next;
	}

	return num_caps;
}

error_t negotiate_features(pci::device& virtio_dev, virtio_pci_common_cfg* cfg)
{
	for (int i = 0; i < 2; i++) {
		cfg->device_feature_select = i;
		cfg->driver_feature_select = i;
		cfg->driver_feature = cfg->device_feature;
	}

	cfg->device_status |= VIRTIO_STATUS_FEATURES_OK;
	if ((cfg->device_status & VIRTIO_STATUS_FEATURES_OK) == 0) {
		printk(KERN_ERROR, "Virtio device does not support features");
	}

	return OK;
}

error_t setup_virtqueue(pci::device& virtio_dev, virtio_pci_common_cfg* cfg)
{
	auto descriptor_area_size = cfg->queue_size * 16;
	auto driver_ring_size = cfg->queue_size * 2 + 6;
	auto device_ring_size = cfg->queue_size * 8 + 6;

	auto driver_ring_offset = descriptor_area_size;
	auto device_ring_offset =
			align_up(driver_ring_offset + driver_ring_size, PAGE_SIZE);

	auto total_size = device_ring_offset + align_up(device_ring_size, PAGE_SIZE);

	for (int i = 0; i < cfg->num_queues; ++i) {
		cfg->queue_select = i;

		void* addr = kmalloc(total_size, KMALLOC_ZEROED, PAGE_SIZE);
		if (addr == nullptr) {
			printk(KERN_ERROR, "Failed to allocate memory for virtio queue");
			return ERR_NO_MEMORY;
		}

		cfg->queue_desc = reinterpret_cast<uint64_t>(addr);
		cfg->queue_driver = reinterpret_cast<uint64_t>(addr) + driver_ring_offset;
		cfg->queue_device = reinterpret_cast<uint64_t>(addr) + device_ring_offset;

		cfg->queue_msix_vector = static_cast<uint16_t>(interrupt_vector::VIRTQUEUE);
		if (cfg->queue_msix_vector != interrupt_vector::VIRTQUEUE) {
			printk(KERN_ERROR, "queue_msix_vector: 0x%x", cfg->queue_msix_vector);
			printk(KERN_ERROR, "Failed to allocate MSI-X vector for virtqueue");
			return ERR_NO_MEMORY;
		}
	}

	return OK;
}

error_t configure_pci_common_cfg(pci::device& virtio_dev, virtio_pci_common_cfg* cfg)
{
	cfg->device_status = 0;
	while (cfg->device_status != 0) {
	}

	cfg->device_status = VIRTIO_STATUS_ACKNOWLEDGE;
	cfg->device_status |= VIRTIO_STATUS_DRIVER;

	cfg->config_msix_vector = interrupt_vector::VIRTIO;

	if (auto err = negotiate_features(virtio_dev, cfg); IS_ERR(err)) {
		printk(KERN_ERROR, "Failed to negotiate features");
		return err;
	}

	if (auto err = setup_virtqueue(virtio_dev, cfg); IS_ERR(err)) {
		printk(KERN_ERROR, "Failed to setup virtqueue");
		return err;
	}

	printk(KERN_ERROR, "initialized virtio device");

	return OK;
}

error_t set_virtio_pci_capability(pci::device& virtio_dev, virtio_pci_cap* caps)
{
	while (caps != nullptr) {
		switch (caps->first_dword.fields.cfg_type) {
			case VIRTIO_PCI_CAP_COMMON_CFG: {
				printk(KERN_ERROR, "found VIRTIO_PCI_CAP_COMMON_CFG");

				auto* cfg = get_virtio_pci_capability<virtio_pci_common_cfg>(
						virtio_dev, caps);
				configure_pci_common_cfg(virtio_dev, cfg);

				break;
			}

			case VIRTIO_PCI_CAP_NOTIFY_CFG:
				printk(KERN_ERROR, "found VIRTIO_PCI_CAP_NOTIFY_CFG");
				break;
			case VIRTIO_PCI_CAP_ISR_CFG:
				printk(KERN_ERROR, "found VIRTIO_PCI_CAP_ISR_CFG");
				break;
			case VIRTIO_PCI_CAP_DEVICE_CFG:
				printk(KERN_ERROR, "found VIRTIO_PCI_CAP_DEVICE_CFG");
				break;
			case VIRTIO_PCI_CAP_PCI_CFG:
				printk(KERN_ERROR, "found VIRTIO_PCI_CAP_PCI_CFG");
				break;
			default:
				printk(KERN_ERROR, "Unknown virtio pci cap");
		}

		caps = caps->next;
	}

	return OK;
}
