#include "task/builtin.hpp"
#include "file_system/fat.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "hardware/usb/xhci/xhci.hpp"
#include "libs/common/message.hpp"
#include "libs/common/types.hpp"
#include "memory/page.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <cstddef>

void task_idle()
{
	while (true) {
		__asm__("hlt");
	}
}

void task_kernel()
{
	task* t = CURRENT_TASK;

	t->message_handlers[IPC_INITIALIZE_TASK] = +[](const message& m) {
		message send_m = { .type = IPC_INITIALIZE_TASK, .sender = KERNEL_TASK_ID };
		send_m.data.init.task_id = m.sender;
		send_message(SHELL_TASK_ID, &send_m);
	};

	t->message_handlers[IPC_MEMORY_USAGE] = +[](const message& m) {
		message send_m = { .type = IPC_MEMORY_USAGE, .sender = KERNEL_TASK_ID };

		size_t used_mem = 0;
		size_t total_mem = 0;

		get_memory_usage(&total_mem, &used_mem);

		send_m.data.memory_usage.total = total_mem;
		send_m.data.memory_usage.used = used_mem;

		send_message(m.sender, &send_m);
	};

	process_messages(t);
}

void task_shell()
{
	auto* entry = file_system::find_directory_entry_by_path("/shell");
	if (entry == nullptr) {
		printk(KERN_ERROR, "failed to find /shell");
		return;
	}

	file_system::execute_file(*entry, nullptr);
}

void task_file_system()
{
	task* t = CURRENT_TASK;

	t->message_handlers[IPC_FILE_SYSTEM_OPERATION] = +[](const message& m) {
		message send_m;
		memset(&send_m.data.write_shell.buf, 0, sizeof(send_m.data.write_shell.buf));

		switch (m.data.fs_operation.operation) {
			case FS_OP_LIST: {
				send_m.type = NOTIFY_WRITE;
				send_m.data.write_shell.is_end_of_message = true;

				char path[30];
				memcpy(path, m.data.fs_operation.path, 30);

				auto* entry = file_system::find_directory_entry_by_path(path);
				if (entry == nullptr) {
					memcpy(send_m.data.write_shell.buf,
						   "ls: No such file or directory", 30);
					send_message(SHELL_TASK_ID, &send_m);
					return;
				}

				if (entry->attribute == file_system::entry_attribute::ARCHIVE) {
					memcpy(send_m.data.write_shell.buf, path, strlen(path));
					send_message(SHELL_TASK_ID, &send_m);
					return;
				}

				char buf[128];
				memset(buf, 0, 128);
				int buf_index = 0;
				size_t entry_count = 0;
				auto entries = file_system::list_entries_in_directory(entry);
				for (auto* e : entries) {
					++entry_count;

					char name[13];
					file_system::read_dir_entry_name(*e, name);
					to_lower(name);

					size_t len = strlen(name);

					memcpy(buf + buf_index, name, len);

					if (entry_count != entries.size()) {
						buf[buf_index + len] = '\n';
					}

					buf_index += len + 1;
				}

				memcpy(send_m.data.write_shell.buf, buf, 128);

				send_message(SHELL_TASK_ID, &send_m);
				break;
			}

			case FS_OP_READ: {
				send_m.type = NOTIFY_WRITE;
				send_m.data.write_shell.is_end_of_message = true;

				char path[30];
				memcpy(path, m.data.fs_operation.path, 30);

				auto* entry = file_system::find_directory_entry_by_path(path);
				if (entry == nullptr) {
					memcpy(send_m.data.write_shell.buf,
						   "cat: No such file or directory", 30);
					send_message(SHELL_TASK_ID, &send_m);
					return;
				}

				file_system::load_file(send_m.data.write_shell.buf, entry->file_size,
									   *entry);
				send_m.data.write_shell.buf[entry->file_size] = '\0';

				send_message(SHELL_TASK_ID, &send_m);
				break;
			}

			default:
				break;
		}
	};

	process_messages(t);
}

void task_usb_handler()
{
	task* t = CURRENT_TASK;

	t->message_handlers[NOTIFY_XHCI] =
			+[](const message& m) { usb::xhci::process_events(); };

	process_messages(t);
}
