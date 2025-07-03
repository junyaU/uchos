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

namespace kernel::hw::virtio {

size_t find_virtio_pci_cap(virtio_pci_device& virtio_dev)
{
	uint8_t cap_id, cap_next;
	uint32_t cap_addr = kernel::hw::pci::get_capability_pointer(*virtio_dev.dev);
	virtio_pci_cap* prev_cap = nullptr;
	size_t num_caps = 0;
	virtio_dev.caps = nullptr;

	while (cap_addr != 0) {
		auto header = kernel::hw::pci::read_capability_header(*virtio_dev.dev, cap_addr);
		cap_id = header.bits.cap_id;
		cap_next = header.bits.next_ptr;

		if (cap_id == kernel::hw::pci::CAP_VIRTIO) {
			void* addr = kernel::memory::kmalloc(sizeof(virtio_pci_cap), kernel::memory::KMALLOC_ZEROED);
			virtio_pci_cap* cap = new (addr) virtio_pci_cap;
			cap->first_dword.data = kernel::hw::pci::read_conf_reg(*virtio_dev.dev, cap_addr);
			cap->second_dword.data =
					kernel::hw::pci::read_conf_reg(*virtio_dev.dev, cap_addr + 4);
			cap->offset = kernel::hw::pci::read_conf_reg(*virtio_dev.dev, cap_addr + 8);
			cap->length = kernel::hw::pci::read_conf_reg(*virtio_dev.dev, cap_addr + 12);

			if (prev_cap != nullptr) {
				prev_cap->next = cap;
			} else {
				virtio_dev.caps = cap;
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
	uint64_t driver_features = 0;
	uint64_t device_features = 0;

	for (int i = 0; i < 2; ++i) {
		virtio_dev.common_cfg->device_feature_select = i;
		device_features |= (uint64_t)virtio_dev.common_cfg->device_feature
						   << (i * 32);
	}

	if ((device_features & (1ULL << VIRTIO_F_VERSION_1)) != 0) {
		driver_features |= (1ULL << VIRTIO_F_VERSION_1);
	}

	for (int i = 0; i < 2; ++i) {
		virtio_dev.common_cfg->driver_feature_select = i;
		virtio_dev.common_cfg->driver_feature =
				(driver_features >> (i * 32)) & 0xffffffff;
	}

	virtio_dev.common_cfg->device_status |= VIRTIO_STATUS_FEATURES_OK;
	if ((virtio_dev.common_cfg->device_status & VIRTIO_STATUS_FEATURES_OK) == 0) {
		LOG_ERROR("Virtio device does not support features");
	}

	return OK;
}

error_t setup_virtqueue(virtio_pci_device& virtio_dev)
{
	size_t num_desc = virtio_dev.common_cfg->queue_size;
	size_t descriptor_area_size = calc_desc_area_size(num_desc);
	size_t driver_ring_size = calc_driver_ring_size(num_desc);
	size_t device_ring_size = calc_device_ring_size(num_desc);

	auto driver_ring_offset = align_up(descriptor_area_size, 2);
	auto device_ring_offset = align_up(driver_ring_offset + driver_ring_size, 4);

	size_t total_size = device_ring_offset + device_ring_size;
	virtio_dev.queues = reinterpret_cast<virtio_virtqueue*>(
			kernel::memory::kmalloc(sizeof(virtio_virtqueue) * virtio_dev.common_cfg->num_queues,
					kernel::memory::KMALLOC_ZEROED));
	if (virtio_dev.queues == nullptr) {
		return ERR_NO_MEMORY;
	}

	for (int i = 0; i < virtio_dev.common_cfg->num_queues; ++i) {
		virtio_dev.common_cfg->queue_select = i;

		void* addr = kernel::memory::kmalloc(total_size, kernel::memory::KMALLOC_ZEROED, kernel::memory::PAGE_SIZE);
		if (addr == nullptr) {
			return ERR_NO_MEMORY;
		}

		uint64_t desc_addr = reinterpret_cast<uint64_t>(addr);
		uint64_t driver_ring_addr = desc_addr + driver_ring_offset;
		uint64_t device_ring_addr = desc_addr + device_ring_offset;

		virtio_dev.common_cfg->queue_desc = desc_addr;
		virtio_dev.common_cfg->queue_driver = driver_ring_addr;
		virtio_dev.common_cfg->queue_device = device_ring_addr;

		init_virtqueue(&virtio_dev.queues[i], i, num_desc, desc_addr,
					   driver_ring_addr, device_ring_addr);

		virtio_dev.common_cfg->queue_msix_vector = 1;
		if (virtio_dev.common_cfg->queue_msix_vector == NO_VECTOR) {
			LOG_ERROR("Failed to allocate MSI-X vector for virtqueue");
			return ERR_NO_MEMORY;
		}

		virtio_dev.common_cfg->queue_enable = 1;
	}

	return OK;
}

error_t configure_pci_common_cfg(virtio_pci_device& virtio_dev)
{
	virtio_dev.common_cfg =
			get_virtio_pci_capability<virtio_pci_common_cfg>(virtio_dev);

	virtio_dev.common_cfg->device_status = 0;
	while (virtio_dev.common_cfg->device_status != 0) {
	}

	virtio_dev.common_cfg->device_status = VIRTIO_STATUS_ACKNOWLEDGE;
	virtio_dev.common_cfg->device_status |= VIRTIO_STATUS_DRIVER;

	ASSERT_OK(negotiate_features(virtio_dev));

	virtio_dev.common_cfg->config_msix_vector = 0;
	if (virtio_dev.common_cfg->config_msix_vector == NO_VECTOR) {
		LOG_ERROR("Failed to allocate MSI-X vector for virtio device");
		return ERR_NO_MEMORY;
	}

	ASSERT_OK(setup_virtqueue(virtio_dev));

	virtio_dev.common_cfg->device_status |= VIRTIO_STATUS_DRIVER_OK;

	return OK;
}

error_t configure_pci_notify_cfg(virtio_pci_device& virtio_dev)
{

	uint64_t bar_addr = kernel::hw::pci::read_base_address_register(
			*virtio_dev.dev, virtio_dev.notify_cfg->cap.second_dword.fields.bar);

	bar_addr = bar_addr & ~0xfff;

	virtio_dev.notify_base = bar_addr + virtio_dev.notify_cfg->cap.offset;

	virtio_dev.notify_base +=
			static_cast<uintptr_t>(virtio_dev.notify_cfg->notify_off_multiplier *
								   virtio_dev.common_cfg->num_queues);

	return OK;
}

error_t set_virtio_pci_capability(virtio_pci_device& virtio_dev)
{
	while (virtio_dev.caps != nullptr) {
		switch (virtio_dev.caps->first_dword.fields.cfg_type) {
			case VIRTIO_PCI_CAP_COMMON_CFG:
				configure_pci_common_cfg(virtio_dev);
				break;

			case VIRTIO_PCI_CAP_NOTIFY_CFG:
				virtio_dev.notify_cfg =
						reinterpret_cast<virtio_pci_notify_cap*>(virtio_dev.caps);
				break;

			case VIRTIO_PCI_CAP_ISR_CFG:
				LOG_INFO("ISR CFG not supported");
				break;

			case VIRTIO_PCI_CAP_DEVICE_CFG:
				LOG_INFO("Device CFG not supported");
				break;

			case VIRTIO_PCI_CAP_PCI_CFG:
				LOG_INFO("PCI CFG not supported");
				break;

			default:
				LOG_ERROR("Unknown virtio pci cap");
		}

		virtio_dev.caps = virtio_dev.caps->next;
	}

	ASSERT_OK(configure_pci_notify_cfg(virtio_dev));

	return OK;
}

void notify_virtqueue(virtio_pci_device& virtio_dev, size_t queue_idx)
{
	asm volatile("sfence" ::: "memory");
	*(volatile uint32_t*)virtio_dev.notify_base = queue_idx;
}

} // namespace kernel::hw::virtio
