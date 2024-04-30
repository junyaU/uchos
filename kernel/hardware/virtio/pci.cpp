#include "hardware/virtio/pci.hpp"
#include "hardware/pci.hpp"
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
