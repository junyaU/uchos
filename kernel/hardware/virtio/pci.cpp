#include "hardware/virtio/pci.hpp"
#include "bit_utils.hpp"
#include "graphics/log.hpp"
#include "hardware/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

size_t find_virtio_pci_cap(virtio_pci_device& virtio_dev, virtio_pci_cap** caps)
{
	uint8_t cap_id, cap_next;
	uint32_t cap_addr = pci::get_capability_pointer(*virtio_dev.dev);
	virtio_pci_cap* prev_cap = nullptr;
	size_t num_caps = 0;
	*caps = nullptr;

	while (cap_addr != 0) {
		auto header = pci::read_capability_header(*virtio_dev.dev, cap_addr);
		cap_id = header.bits.cap_id;
		cap_next = header.bits.next_ptr;

		if (cap_id == pci::CAP_VIRTIO) {
			void* addr = kmalloc(sizeof(virtio_pci_cap), KMALLOC_ZEROED);
			virtio_pci_cap* cap = new (addr) virtio_pci_cap;
			cap->first_dword.data = pci::read_conf_reg(*virtio_dev.dev, cap_addr);
			cap->second_dword.data =
					pci::read_conf_reg(*virtio_dev.dev, cap_addr + 4);
			cap->offset = pci::read_conf_reg(*virtio_dev.dev, cap_addr + 8);
			cap->length = pci::read_conf_reg(*virtio_dev.dev, cap_addr + 12);

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

error_t negotiate_features(virtio_pci_device& virtio_dev)
{
	for (int i = 0; i < 2; ++i) {
		virtio_dev.common_cfg->device_feature_select = i;
		virtio_dev.common_cfg->driver_feature_select = i;
		virtio_dev.common_cfg->driver_feature =
				virtio_dev.common_cfg->device_feature;
	}

	virtio_dev.common_cfg->device_status |= VIRTIO_STATUS_FEATURES_OK;
	if ((virtio_dev.common_cfg->device_status & VIRTIO_STATUS_FEATURES_OK) == 0) {
		printk(KERN_ERROR, "Virtio device does not support features");
	}

	return OK;
}

error_t setup_virtqueue(virtio_pci_device& virtio_dev)
{
	size_t num_desc = virtio_dev.common_cfg->queue_size;
	auto descriptor_area_size = calc_desc_area_size(num_desc);
	auto driver_ring_size = calc_driver_ring_size(num_desc);
	auto device_ring_size = calc_device_ring_size(num_desc);

	auto driver_ring_offset = descriptor_area_size;
	auto device_ring_offset =
			align_up(driver_ring_offset + driver_ring_size, PAGE_SIZE);

	auto total_size = device_ring_offset + align_up(device_ring_size, PAGE_SIZE);

	virtio_dev.queues = reinterpret_cast<virtio_virtqueue*>(
			kmalloc(sizeof(virtio_virtqueue) * virtio_dev.common_cfg->num_queues,
					KMALLOC_ZEROED));
	if (virtio_dev.queues == nullptr) {
		printk(KERN_ERROR, "Failed to allocate memory for virtio queues");
		return ERR_NO_MEMORY;
	}

	for (int i = 0; i < virtio_dev.common_cfg->num_queues; ++i) {
		virtio_dev.common_cfg->queue_select = i;

		void* addr = kmalloc(total_size, KMALLOC_ZEROED, PAGE_SIZE);
		if (addr == nullptr) {
			printk(KERN_ERROR, "Failed to allocate memory for virtio queue");
			return ERR_NO_MEMORY;
		}

		uint64_t driver_ring_addr =
				reinterpret_cast<uint64_t>(addr) + driver_ring_offset;
		uint64_t device_ring_addr =
				reinterpret_cast<uint64_t>(addr) + device_ring_offset;

		virtio_dev.common_cfg->queue_desc = reinterpret_cast<uint64_t>(addr);
		virtio_dev.common_cfg->queue_driver = driver_ring_addr;
		virtio_dev.common_cfg->queue_device = device_ring_addr;

		virtio_dev.common_cfg->queue_enable = 1;

		init_virtqueue(&virtio_dev.queues[i], i, num_desc,
					   reinterpret_cast<uintptr_t>(addr), driver_ring_addr,
					   device_ring_addr);

		virtio_dev.common_cfg->queue_msix_vector = 1;
		if (virtio_dev.common_cfg->queue_msix_vector == NO_VECTOR) {
			printk(KERN_ERROR, "queue_msix_vector: 0x%x",
				   virtio_dev.common_cfg->queue_msix_vector);
			printk(KERN_ERROR, "Failed to allocate MSI-X vector for virtqueue");
			return ERR_NO_MEMORY;
		}
	}

	return OK;
}

error_t configure_pci_common_cfg(virtio_pci_device& virtio_dev)
{
	virtio_dev.common_cfg->device_status = 0;
	while (virtio_dev.common_cfg->device_status != 0) {
	}

	virtio_dev.common_cfg->device_status = VIRTIO_STATUS_ACKNOWLEDGE;
	virtio_dev.common_cfg->device_status |= VIRTIO_STATUS_DRIVER;

	if (auto err = negotiate_features(virtio_dev); IS_ERR(err)) {
		printk(KERN_ERROR, "Failed to negotiate features");
		return err;
	}

	virtio_dev.common_cfg->config_msix_vector = 0;
	if (virtio_dev.common_cfg->config_msix_vector == NO_VECTOR) {
		printk(KERN_ERROR, "config_msix_vector: 0x%x",
			   virtio_dev.common_cfg->config_msix_vector);
		printk(KERN_ERROR, "Failed to allocate MSI-X vector for virtio device");
		return ERR_NO_MEMORY;
	}

	if (auto err = setup_virtqueue(virtio_dev); IS_ERR(err)) {
		printk(KERN_ERROR, "Failed to setup virtqueue");
		return err;
	}

	virtio_dev.common_cfg->device_status |= VIRTIO_STATUS_DRIVER_OK;

	printk(KERN_ERROR, "initialized virtio device");

	return OK;
}

error_t configure_pci_notify_cfg(virtio_pci_device& virtio_dev)
{

	uint64_t bar_addr = pci::read_base_address_register(
			*virtio_dev.dev, virtio_dev.notify_cfg->cap.second_dword.fields.bar);

	bar_addr = bar_addr & 0xffff'ffff'ffff'f000U;

	virtio_dev.notify_base = bar_addr + virtio_dev.notify_cfg->cap.offset;

	virtio_dev.notify_base +=
			static_cast<uint64_t>(virtio_dev.common_cfg->queue_notify_off *
								  virtio_dev.notify_cfg->notify_off_multiplier);

	return OK;
}

error_t set_virtio_pci_capability(virtio_pci_device& virtio_dev)
{
	while (virtio_dev.caps != nullptr) {
		switch (virtio_dev.caps->first_dword.fields.cfg_type) {
			case VIRTIO_PCI_CAP_COMMON_CFG: {
				virtio_dev.common_cfg =
						get_virtio_pci_capability<virtio_pci_common_cfg>(virtio_dev);

				configure_pci_common_cfg(virtio_dev);
				break;
			}

			case VIRTIO_PCI_CAP_NOTIFY_CFG:
				virtio_dev.notify_cfg =
						reinterpret_cast<virtio_pci_notify_cap*>(virtio_dev.caps);

				break;
			case VIRTIO_PCI_CAP_ISR_CFG:
				break;
			case VIRTIO_PCI_CAP_DEVICE_CFG:
				break;
			case VIRTIO_PCI_CAP_PCI_CFG:
				break;
			default:
				printk(KERN_ERROR, "Unknown virtio pci cap");
		}

		virtio_dev.caps = virtio_dev.caps->next;
	}

	configure_pci_notify_cfg(virtio_dev);

	return OK;
}

void notify_virtqueue(virtio_pci_device& virtio_dev, size_t queue_idx)
{
	asm volatile("sfence" ::: "memory");
	*(volatile uint32_t*)virtio_dev.notify_base = queue_idx;
}
