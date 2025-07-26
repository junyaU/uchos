#include "pci_device.hpp"
#include <cstdio>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

int main(int argc, char** argv)
{
	pid_t pid = sys_getpid();
	Message m = { .type = MsgType::IPC_PCI, .sender = ProcessId::from_raw(pid) };
	send_message(process_ids::KERNEL, &m);

	Message msg;
	while (true) {
		receive_message(&msg);
		if (msg.type == MsgType::NO_TASK) {
			continue;
		}

		if (msg.type != MsgType::IPC_PCI) {
			continue;
		}

		Message send_m = {
			.type = MsgType::NOTIFY_WRITE,
			.sender = ProcessId::from_raw(pid),
		};

		char device_buf[100];
		output_target_device(device_buf, 100, msg.data.pci.vendor_id, msg.data.pci.device_id);

		char buf[100];
		sprintf(buf, "%s %s\n", msg.data.pci.bus_address, device_buf);
		memcpy(send_m.data.write_shell.buf, buf, strlen(buf));

		send_message(process_ids::SHELL, &send_m);
	}

	return 0;
}
