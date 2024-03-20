#include "task.hpp"
#include "../graphics/font.hpp"
#include "../task/task.hpp"
#include "fat.hpp"

void task_file_system()
{
	task* t = CURRENT_TASK;

	t->message_handlers[IPC_FILE_SYSTEM_OPERATION] = +[](const message& m) {
		if (m.data.fs_operation.operation == FS_OP_LIST) {
			message send_m;
			send_m.type = NOTIFY_WRITE;
			send_m.data.write_shell.is_end_of_message = true;

			char path[30];
			memcpy(path, m.data.fs_operation.path, 30);

			auto* entry = file_system::find_directory_entry_by_path(path);
			if (entry == nullptr) {
				memcpy(send_m.data.write_shell.buf, "ls: No such file or directory",
					   30);
				send_message(SHELL_TASK_ID, &send_m);
				return;
			}

			if (entry->attribute == file_system::entry_attribute::ARCHIVE) {
				memcpy(send_m.data.write_shell.buf, path, strlen(path));
				send_message(SHELL_TASK_ID, &send_m);
				return;
			}

			int i = 0;
			char buf[128];
			auto entries = file_system::list_entries_in_directory(entry);
			for (const auto* e : entries) {
				char name[13];
				file_system::read_dir_entry_name(*e, name);
				to_lower(name);

				size_t len = strlen(name);

				memcpy(buf + i, name, len);
				buf[i + len] = '\n';
				i += len + 1;
			}

			memcpy(send_m.data.write_shell.buf, buf, 128);

			send_message(SHELL_TASK_ID, &send_m);
		} else if (m.data.fs_operation.operation == FS_OP_READ) {
			message send_m;
			send_m.type = NOTIFY_WRITE;
			send_m.data.write_shell.is_end_of_message = true;

			char path[30];
			memcpy(path, m.data.fs_operation.path, 30);

			auto* entry = file_system::find_directory_entry_by_path(path);
			if (entry == nullptr) {
				memcpy(send_m.data.write_shell.buf, "cat: No such file or directory",
					   30);
				send_message(SHELL_TASK_ID, &send_m);
				return;
			}

			file_system::load_file(send_m.data.write_shell.buf, entry->file_size,
								   *entry);
			send_m.data.write_shell.buf[entry->file_size] = '\0';

			send_message(SHELL_TASK_ID, &send_m);
		}
	};

	process_messages(t);
}
