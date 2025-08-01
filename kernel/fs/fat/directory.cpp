/**
 * @file directory.cpp
 * @brief FAT32 directory operations implementation
 */

#include "fat.hpp"
#include "fs/path.hpp"
#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "internal_common.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/stat.hpp>
#include <libs/common/types.hpp>
#include <map>
#include <string>
#include <vector>

namespace kernel::fs::fat
{

std::map<fs_id_t, ProcessId> change_dir_requests;
std::map<fs_id_t, std::string> change_dir_names;
std::map<fs_id_t, std::string> parent_dir_names;
fs_id_t next_change_dir_id = 1000000;

namespace
{

void update_directory_path_from_parent(kernel::task::Task* t, const std::string& parent_path)
{
	strncpy(t->fs_path.full_path, parent_path.c_str(), sizeof(t->fs_path.full_path) - 1);
	t->fs_path.full_path[sizeof(t->fs_path.full_path) - 1] = '\0';

	if (parent_path == "/") {
		t->fs_path.current_dir_name[0] = '/';
		t->fs_path.current_dir_name[1] = '\0';
		return;
	}

	const size_t last_slash = parent_path.rfind('/');
	if (last_slash != std::string::npos) {
		const std::string dir_name = parent_path.substr(last_slash + 1);
		strncpy(t->fs_path.current_dir_name, dir_name.c_str(), 12);
		t->fs_path.current_dir_name[12] = '\0';
	}
}

void update_directory_path_append(kernel::task::Task* t, const std::string& dir_name)
{
	strncpy(t->fs_path.current_dir_name, dir_name.c_str(), 12);
	t->fs_path.current_dir_name[12] = '\0';

	if (strcmp(t->fs_path.full_path, "/") == 0) {
		snprintf(t->fs_path.full_path, sizeof(t->fs_path.full_path), "/%s", dir_name.c_str());
	} else {
		const size_t len = strlen(t->fs_path.full_path);
		snprintf(t->fs_path.full_path + len,
		         sizeof(t->fs_path.full_path) - len,
		         "/%s",
		         dir_name.c_str());
	}
}

void change_to_root_directory(kernel::task::Task* t)
{
	if (t->fs_path.current_dir != nullptr && t->fs_path.current_dir != ROOT_DIR) {
		kernel::memory::free(t->fs_path.current_dir);
	}

	t->fs_path.current_dir = ROOT_DIR;
	t->fs_path.current_dir_name[0] = '/';
	t->fs_path.current_dir_name[1] = '\0';
	strncpy(t->fs_path.full_path, "/", sizeof(t->fs_path.full_path) - 1);
	t->fs_path.full_path[sizeof(t->fs_path.full_path) - 1] = '\0';
}

bool validate_change_dir_request(const Message& m, kernel::task::Task*& task)
{
	auto it = change_dir_requests.find(m.data.blk_io.request_id);
	if (it == change_dir_requests.end()) {
		LOG_ERROR("request_id not found in change_dir_requests");
		return false;
	}

	task = kernel::task::get_task(it->second);
	if (task == nullptr) {
		LOG_ERROR("task not found");
		change_dir_requests.erase(it);
		return false;
	}

	return true;
}

void update_directory_entry(kernel::task::Task* t, DirectoryEntry* new_dir)
{
	if (t->fs_path.current_dir != nullptr && t->fs_path.current_dir != ROOT_DIR) {
		kernel::memory::free(t->fs_path.current_dir);
	}
	t->fs_path.current_dir = new_dir;
}

void finalize_directory_change(const Message& m, kernel::task::Task* t)
{
	auto name_it = change_dir_names.find(m.data.blk_io.request_id);
	if (name_it != change_dir_names.end()) {
		if (name_it->second == "..") {
			auto parent_name_it = parent_dir_names.find(m.data.blk_io.request_id);
			if (parent_name_it != parent_dir_names.end()) {
				update_directory_path_from_parent(t, parent_name_it->second);
				parent_dir_names.erase(parent_name_it);
			} else if (t->fs_path.current_dir == ROOT_DIR) {
				change_to_root_directory(t);
			}
		} else {
			update_directory_path_append(t, name_it->second);
		}
		change_dir_names.erase(name_it);
	}
	change_dir_requests.erase(m.data.blk_io.request_id);
}

void handle_virtio_response(const Message& m)
{
	kernel::task::Task* task = nullptr;
	if (!validate_change_dir_request(m, task)) {
		return;
	}

	update_directory_entry(task, reinterpret_cast<DirectoryEntry*>(m.data.blk_io.buf));
	finalize_directory_change(m, task);
}

void change_to_root(const Message& m, kernel::task::Task* t)
{
	change_to_root_directory(t);

	Message reply = { .type = MsgType::FS_CHANGE_DIR, .sender = process_ids::FS_FAT32 };
	reply.data.fs.name[0] = '/';
	reply.data.fs.name[1] = '\0';
	reply.data.fs.result = 0;
	kernel::task::send_message(m.sender, reply);
}

void send_error_response(const Message& m, const char* error_msg = nullptr)
{
	Message reply = { .type = MsgType::FS_CHANGE_DIR, .sender = process_ids::FS_FAT32 };
	reply.data.fs.result = -1;
	if (error_msg != nullptr) {
		strncpy(reply.data.fs.name, error_msg, sizeof(reply.data.fs.name) - 1);
		reply.data.fs.name[sizeof(reply.data.fs.name) - 1] = '\0';
	}
	kernel::task::send_message(m.sender, reply);
}

void request_parent_directory_load(const Message& m,
                                   kernel::task::Task* t,
                                   DirectoryEntry* parent_entry)
{
	const fs_id_t request_id = next_change_dir_id++;
	change_dir_requests[request_id] = t->id;
	change_dir_names[request_id] = "..";

	const std::string current_full_path = t->fs_path.full_path;
	const size_t last_slash = current_full_path.rfind('/');
	parent_dir_names[request_id] =
	    (last_slash == 0) ? "/" : current_full_path.substr(0, last_slash);

	send_read_req_to_blk_device(calc_start_sector(parent_entry->first_cluster()),
	                            BYTES_PER_CLUSTER,
	                            MsgType::FS_CHANGE_DIR,
	                            request_id);

	Message reply = { .type = MsgType::FS_CHANGE_DIR, .sender = process_ids::FS_FAT32 };
	if (parent_dir_names[request_id] == "/") {
		memcpy(reply.data.fs.name, "/", 2);
	} else {
		memcpy(reply.data.fs.name,
		       parent_dir_names[request_id].c_str(),
		       parent_dir_names[request_id].length() + 1);
	}
	reply.data.fs.result = 0;
	kernel::task::send_message(m.sender, reply);
}

void handle_parent_directory_change(const Message& m, kernel::task::Task* t)
{
	if (t->fs_path.current_dir == ROOT_DIR) {
		change_to_root(m, t);
		return;
	}

	DirectoryEntry* parent_entry = find_dir_entry(t->fs_path.current_dir, "..");
	if (parent_entry == nullptr) {
		send_error_response(m);
		return;
	}

	if (parent_entry->first_cluster() == 0) {
		change_to_root(m, t);
		return;
	}

	request_parent_directory_load(m, t, parent_entry);
}

void handle_normal_directory_change(const Message& m, kernel::task::Task* t, const char* path_name)
{
	char upper_name[256];
	strncpy(upper_name, path_name, sizeof(upper_name) - 1);
	upper_name[sizeof(upper_name) - 1] = '\0';
	kernel::graphics::to_upper(upper_name);

	DirectoryEntry* entry = find_dir_entry(t->fs_path.current_dir, upper_name);
	if (entry == nullptr || entry->attribute != entry_attribute::DIRECTORY) {
		send_error_response(m);
		return;
	}

	const fs_id_t request_id = next_change_dir_id++;
	change_dir_requests[request_id] = t->id;
	change_dir_names[request_id] = upper_name;

	send_read_req_to_blk_device(calc_start_sector(entry->first_cluster()),
	                            BYTES_PER_CLUSTER,
	                            MsgType::FS_CHANGE_DIR,
	                            request_id);

	Message reply = { .type = MsgType::FS_CHANGE_DIR, .sender = process_ids::FS_FAT32 };
	memcpy(reply.data.fs.name, upper_name, strlen(upper_name) + 1);
	reply.data.fs.result = 0;
	kernel::task::send_message(m.sender, reply);
}

kernel::task::Task* get_effective_task(ProcessId sender)
{
	auto* t = kernel::task::get_task(sender);
	if (t != nullptr && t->parent_id != process_ids::INVALID) {
		return kernel::task::get_task(t->parent_id);
	}
	return t;
}

}  // namespace

void handle_fs_change_dir(const Message& m)
{
	if (m.sender == process_ids::VIRTIO_BLK) {
		handle_virtio_response(m);
		return;
	}

	auto* task = get_effective_task(m.sender);
	if (task == nullptr) {
		send_error_response(m, "Task not found");
		return;
	}

	const char* path_name = reinterpret_cast<const char*>(m.data.fs.name);

	if (strcmp(path_name, "/") == 0) {
		change_to_root(m, task);
		return;
	}

	if (strcmp(path_name, "..") == 0) {
		handle_parent_directory_change(m, task);
		return;
	}

	handle_normal_directory_change(m, task, path_name);
}

void handle_get_directory_contents(const Message& m)
{
	if (m.type == MsgType::IPC_READ_FROM_BLK_DEVICE) {
		return;
	}

	auto* t = kernel::task::get_task(m.sender);
	if (t->parent_id != process_ids::INVALID) {
		t = kernel::task::get_task(t->parent_id);
	}

	DirectoryEntry* current_dir = t->fs_path.current_dir;
	std::vector<char> entries(ENTRIES_PER_CLUSTER * sizeof(DirectoryEntry));
	int entries_count = 0;
	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (current_dir[i].name[0] == 0x00) {
			break;
		}

		if (current_dir[i].name[0] == 0xE5) {
			continue;
		}

		if (current_dir[i].attribute == entry_attribute::LONG_NAME) {
			continue;
		}

		memcpy(&entries[entries_count * sizeof(DirectoryEntry)],
		       &current_dir[i],
		       sizeof(DirectoryEntry));
		++entries_count;
	}

	void* buf;
	ALLOC_OR_RETURN(buf, entries_count * sizeof(Stat), kernel::memory::ALLOC_ZEROED);

	for (int i = 0; i < entries_count; ++i) {
		Stat* s = reinterpret_cast<Stat*>(buf) + i;
		read_dir_entry_name(
		    *reinterpret_cast<DirectoryEntry*>(&entries[i * sizeof(DirectoryEntry)]), s->name);
		s->size = current_dir[i].file_size;
		s->type = current_dir[i].attribute == entry_attribute::DIRECTORY
		              ? StatType::DIRECTORY
		              : StatType::REGULAR_FILE;
	}

	Message sm = { .type = MsgType::GET_DIRECTORY_CONTENTS, .sender = process_ids::FS_FAT32 };
	sm.tool_desc.addr = buf;
	sm.tool_desc.size = entries_count * sizeof(Stat);
	sm.tool_desc.present = true;

	kernel::task::send_message(m.sender, sm);
}

void handle_fs_pwd(const Message& m)
{
	Message reply = { .type = MsgType::FS_PWD, .sender = process_ids::FS_FAT32 };

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t->parent_id != process_ids::INVALID) {
		t = kernel::task::get_task(t->parent_id);
	}

	if (t->fs_path.current_dir == nullptr) {
		kernel::task::send_message(m.sender, reply);
		return;
	}

	memcpy(
	    reply.data.fs.name, t->fs_path.current_dir_name, strlen(t->fs_path.current_dir_name) + 1);

	kernel::task::send_message(m.sender, reply);
}

void update_directory_entry_on_disk(DirectoryEntry* entry, const char* name)
{
	if (entry == nullptr || ROOT_DIR == nullptr) {
		LOG_ERROR("Invalid directory entry or ROOT_DIR not initialized");
		return;
	}

	DirectoryEntry* disk_entry = nullptr;
	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (ROOT_DIR[i].name[0] == 0x00) {
			break;
		}

		if (entry_name_is_equal(ROOT_DIR[i], name)) {
			disk_entry = &ROOT_DIR[i];
			break;
		}
	}

	if (disk_entry == nullptr) {
		LOG_ERROR("Directory entry not found in ROOT_DIR");
		return;
	}

	memcpy(disk_entry, entry, sizeof(DirectoryEntry));

	void* write_buffer;
	ALLOC_OR_RETURN(write_buffer, BYTES_PER_CLUSTER, kernel::memory::ALLOC_ZEROED);

	memcpy(write_buffer, ROOT_DIR, BYTES_PER_CLUSTER);

	const unsigned int root_dir_sector = calc_start_sector(VOLUME_BPB->root_cluster);

	// Write the entire ROOT_DIR cluster back to disk
	send_write_req_to_blk_device(
	    write_buffer, root_dir_sector, BYTES_PER_CLUSTER, MsgType::FS_WRITE, 0, 0);

	// Note: The buffer will be freed when the write operation completes
	// TODO: This should be handled in the write completion handler
}

}  // namespace kernel::fs::fat
