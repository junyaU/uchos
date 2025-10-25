#include "hardware/virtio/net.hpp"
#include <libs/common/types.hpp>
#include "graphics/log.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "task/task.hpp"

namespace kernel::hw::virtio
{

VirtioPciDevice* net_dev = nullptr;
uint8_t mac_addr[6] = { 0 };

error_t read_mac_address(VirtioPciDevice& virtio_dev)
{
	VirtioPciCap cap = virtio_dev.device_cfg->cap;
	uint64_t bar_addr = kernel::hw::pci::read_base_address_register(
			*virtio_dev.dev, cap.second_dword.fields.bar);
	bar_addr = bar_addr & ~0xfff;

	VirtioNetConfig* net_cfg =
			reinterpret_cast<VirtioNetConfig*>(bar_addr + cap.offset);

	memcpy(mac_addr, net_cfg->mac, 6);

	LOG_ERROR("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1],
			  mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

	return OK;
}

error_t init_virtio_net_device()
{
	void* buffer = kernel::memory::alloc(sizeof(VirtioPciDevice),
										 kernel::memory::ALLOC_ZEROED);
	if (buffer == nullptr) {
		return ERR_NO_MEMORY;
	}

	net_dev = new (buffer) VirtioPciDevice();
	ASSERT_OK(init_virtio_pci_device(net_dev, VIRTIO_NET));

	kernel::task::CURRENT_TASK->is_initilized = true;

	return OK;
}

void virtio_net_service()
{
	kernel::task::Task* t = kernel::task::CURRENT_TASK;

	init_virtio_net_device();

	read_mac_address(*net_dev);

	kernel::task::process_messages(t);
}
} // namespace kernel::hw::virtio
