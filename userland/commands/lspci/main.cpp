#include <cstdio>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>
#include "pci_device.hpp"

int main(int argc, char** argv)
{
	// One call returns the whole device table as an OOL array; the old
	// receive loop over N separate responses is gone (issue #314 Stage C)
	Message m = make_request(MsgType::KERNEL_PCI_LIST);
	Message msg = call(process_ids::KERNEL, &m);
	if (IS_ERR(msg.result)) {
		printu("lspci: failed to list devices");
		return 0;
	}

	if (msg.ool.size == 0) {
		printu("lspci: no devices");
		return 0;
	}

	const PciDeviceInfo* devices =
			reinterpret_cast<const PciDeviceInfo*>(msg.ool.addr);
	const size_t num_devices = msg.ool.size / sizeof(PciDeviceInfo);

	for (size_t i = 0; i < num_devices; ++i) {
		char device_buf[100];
		output_target_device(device_buf, sizeof(device_buf), devices[i].vendor_id,
							 devices[i].device_id);

		char buf[120];
		sprintf(buf, "%s %s", devices[i].bus_address, device_buf);
		printu(buf);
	}

	ool_release(devices);

	return 0;
}
