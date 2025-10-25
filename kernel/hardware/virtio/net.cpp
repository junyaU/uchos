#include "hardware/virtio/net.hpp"
#include "hardware/virtio/pci.hpp"
#include "hardware/virtio/virtio.hpp"
#include "libs/common/types.hpp"
#include "task/task.hpp"

namespace kernel::hw::virtio {

VirtioPciDevice *net_dev = nullptr;

error_t init_virtio_net_device() {
  void *buffer = kernel::memory::alloc(sizeof(VirtioPciDevice),
                                       kernel::memory::ALLOC_ZEROED);
  if (buffer == nullptr) {
    return ERR_NO_MEMORY;
  }

  net_dev = new (buffer) VirtioPciDevice();
  ASSERT_OK(init_virtio_pci_device(net_dev, VIRTIO_NET));

  kernel::task::CURRENT_TASK->is_initilized = true;

  return OK;
}

void virtio_net_service() {
  kernel::task::Task *t = kernel::task::CURRENT_TASK;

  init_virtio_net_device();

  kernel::task::process_messages(t);
}
} // namespace kernel::hw::virtio