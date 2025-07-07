/**
 * @file directory.cpp
 * @brief FAT32 directory operations implementation
 */

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/stat.hpp>
#include <libs/common/types.hpp>

#include "graphics/font.hpp"
#include "graphics/log.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

#include "../fat.hpp"
#include "../path.hpp"
#include "internal_common.hpp"

namespace kernel::fs::fat
{

// Change directory tracking variables
std::map<fs_id_t, ProcessId> change_dir_requests;
std::map<fs_id_t, std::string> change_dir_names;
std::map<fs_id_t, std::string> parent_dir_names;
fs_id_t next_change_dir_id = 1000000;

namespace
{

void update_directory_path_from_parent(kernel::task::task* t,
									   const std::string& parent_path)
{
	strncpy(t->fs_path.full_path, parent_path.c_str(),
			sizeof(t->fs_path.full_path) - 1);
	t->fs_path.full_path[sizeof(t->fs_path.full_path) - 1] = '\0';

	if (parent_path == "/") {
		t->fs_path.current_dir_name[0] = '/';
		t->fs_path.current_dir_name[1] = '\0';
		return;
	}

	// Get the last component of the path
	const size_t last_slash = parent_path.rfind('/');
	if (last_slash != std::string::npos) {
		const std::string dir_name = parent_path.substr(last_slash + 1);
		strncpy(t->fs_path.current_dir_name, dir_name.c_str(), 12);
		t->fs_path.current_dir_name[12] = '\0';
	}
}

void update_directory_path_append(kernel::task::task* t, const std::string& dir_name)
{
	strncpy(t->fs_path.current_dir_name, dir_name.c_str(), 12);
	t->fs_path.current_dir_name[12] = '\0';

	if (strcmp(t->fs_path.full_path, "/") == 0) {
		// Append to root
		snprintf(t->fs_path.full_path, sizeof(t->fs_path.full_path), "/%s",
				 dir_name.c_str());
	} else {
		// Append to existing path
		const size_t len = strlen(t->fs_path.full_path);
		snprintf(t->fs_path.full_path + len, sizeof(t->fs_path.full_path) - len,
				 "/%s", dir_name.c_str());
	}
}

void change_to_root_directory(kernel::task::task* t)
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

// ディレクトリ変更要求の検証
bool validate_change_dir_request(const message& m, kernel::task::task*& task)
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

// ディレクトリエントリの更新
void update_directory_entry(kernel::task::task* t, directory_entry* new_dir)
{
	if (t->fs_path.current_dir != nullptr && t->fs_path.current_dir != ROOT_DIR) {
		kernel::memory::free(t->fs_path.current_dir);
	}
	t->fs_path.current_dir = new_dir;
}

// ディレクトリ変更の完了処理
void finalize_directory_change(const message& m, kernel::task::task* t)
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

// VirtIOからの応答処理
void handle_virtio_response(const message& m)
{
	kernel::task::task* task = nullptr;
	if (!validate_change_dir_request(m, task)) {
		return;
	}

	update_directory_entry(task,
						   reinterpret_cast<directory_entry*>(m.data.blk_io.buf));
	finalize_directory_change(m, task);
}

// ルートディレクトリへの変更
void change_to_root(const message& m, kernel::task::task* t)
{
	change_to_root_directory(t);

	message reply = { .type = msg_t::FS_CHANGE_DIR,
					  .sender = process_ids::FS_FAT32 };
	reply.data.fs.name[0] = '/';
	reply.data.fs.name[1] = '\0';
	reply.data.fs.result = 0;
	kernel::task::send_message(m.sender, reply);
}

void send_error_response(const message& m, const char* error_msg = nullptr)
{
	message reply = { .type = msg_t::FS_CHANGE_DIR,
					  .sender = process_ids::FS_FAT32 };
	reply.data.fs.result = -1;
	if (error_msg != nullptr) {
		strncpy(reply.data.fs.name, error_msg, sizeof(reply.data.fs.name) - 1);
		reply.data.fs.name[sizeof(reply.data.fs.name) - 1] = '\0';
	}
	kernel::task::send_message(m.sender, reply);
}

// 親ディレクトリのクラスタ読み込み要求
void request_parent_directory_load(const message& m,
								   kernel::task::task* t,
								   directory_entry* parent_entry)
{
	const fs_id_t request_id = next_change_dir_id++;
	change_dir_requests[request_id] = t->id;
	change_dir_names[request_id] = "..";

	// 親パスの計算
	const std::string current_full_path = t->fs_path.full_path;
	const size_t last_slash = current_full_path.rfind('/');
	parent_dir_names[request_id] =
			(last_slash == 0) ? "/" : current_full_path.substr(0, last_slash);

	send_read_req_to_blk_device(calc_start_sector(parent_entry->first_cluster()),
								BYTES_PER_CLUSTER, msg_t::FS_CHANGE_DIR, request_id);

	// 成功応答を送信
	message reply = { .type = msg_t::FS_CHANGE_DIR,
					  .sender = process_ids::FS_FAT32 };
	if (parent_dir_names[request_id] == "/") {
		memcpy(reply.data.fs.name, "/", 2);
	} else {
		memcpy(reply.data.fs.name, parent_dir_names[request_id].c_str(),
			   parent_dir_names[request_id].length() + 1);
	}
	reply.data.fs.result = 0;
	kernel::task::send_message(m.sender, reply);
}

// 親ディレクトリへの変更処理
void handle_parent_directory_change(const message& m, kernel::task::task* t)
{
	// すでにルートディレクトリの場合
	if (t->fs_path.current_dir == ROOT_DIR) {
		change_to_root(m, t);
		return;
	}

	// 親ディレクトリエントリを検索
	directory_entry* parent_entry = find_dir_entry(t->fs_path.current_dir, "..");
	if (parent_entry == nullptr) {
		send_error_response(m);
		return;
	}

	// 親がルートディレクトリの場合
	if (parent_entry->first_cluster() == 0) {
		change_to_root(m, t);
		return;
	}

	// 親ディレクトリの読み込みを要求
	request_parent_directory_load(m, t, parent_entry);
}

// 通常のディレクトリ変更処理
void handle_normal_directory_change(const message& m,
									kernel::task::task* t,
									const char* path_name)
{
	char upper_name[256];
	strncpy(upper_name, path_name, sizeof(upper_name) - 1);
	upper_name[sizeof(upper_name) - 1] = '\0';
	kernel::graphics::to_upper(upper_name);

	directory_entry* entry = find_dir_entry(t->fs_path.current_dir, upper_name);
	if (entry == nullptr || entry->attribute != entry_attribute::DIRECTORY) {
		send_error_response(m);
		return;
	}

	const fs_id_t request_id = next_change_dir_id++;
	change_dir_requests[request_id] = t->id;
	change_dir_names[request_id] = upper_name;

	send_read_req_to_blk_device(calc_start_sector(entry->first_cluster()),
								BYTES_PER_CLUSTER, msg_t::FS_CHANGE_DIR, request_id);

	message reply = { .type = msg_t::FS_CHANGE_DIR,
					  .sender = process_ids::FS_FAT32 };
	memcpy(reply.data.fs.name, upper_name, strlen(upper_name) + 1);
	reply.data.fs.result = 0;
	kernel::task::send_message(m.sender, reply);
}

// タスクの取得（親タスクの考慮を含む）
kernel::task::task* get_effective_task(ProcessId sender)
{
	auto* t = kernel::task::get_task(sender);
	if (t != nullptr && t->parent_id != process_ids::INVALID) {
		return kernel::task::get_task(t->parent_id);
	}
	return t;
}

} // namespace

void handle_fs_change_dir(const message& m)
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

void handle_get_directory_contents(const message& m)
{
	if (m.type == msg_t::IPC_READ_FROM_BLK_DEVICE) {
		return;
	}

	auto* t = kernel::task::get_task(m.sender);
	if (t->parent_id != process_ids::INVALID) {
		t = kernel::task::get_task(t->parent_id);
	}

	directory_entry* current_dir = t->fs_path.current_dir;
	std::vector<char> entries(ENTRIES_PER_CLUSTER * sizeof(directory_entry));
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

		memcpy(&entries[entries_count * sizeof(directory_entry)], &current_dir[i],
			   sizeof(directory_entry));
		++entries_count;
	}

	void* buf =
			kernel::memory::alloc(entries_count * sizeof(stat),
								  kernel::memory::ALLOC_ZEROED, memory::PAGE_SIZE);
	if (buf == nullptr) {
		LOG_ERROR("failed to allocate memory");
		return;
	}

	for (int i = 0; i < entries_count; ++i) {
		stat* s = reinterpret_cast<stat*>(buf) + i;
		read_dir_entry_name(*reinterpret_cast<directory_entry*>(
									&entries[i * sizeof(directory_entry)]),
							s->name);
		s->size = current_dir[i].file_size;
		s->type = current_dir[i].attribute == entry_attribute::DIRECTORY
						  ? stat_type_t::DIRECTORY
						  : stat_type_t::REGULAR_FILE;
	}

	message sm = { .type = msg_t::GET_DIRECTORY_CONTENTS,
				   .sender = process_ids::FS_FAT32 };
	sm.tool_desc.addr = buf;
	sm.tool_desc.size = entries_count * sizeof(stat);
	sm.tool_desc.present = true;

	kernel::task::send_message(m.sender, sm);
}

void handle_fs_pwd(const message& m)
{
	message reply = { .type = msg_t::FS_PWD, .sender = process_ids::FS_FAT32 };

	kernel::task::task* t = kernel::task::get_task(m.sender);
	if (t->parent_id != process_ids::INVALID) {
		t = kernel::task::get_task(t->parent_id);
	}

	if (t->fs_path.current_dir == nullptr) {
		kernel::task::send_message(m.sender, reply);
		return;
	}

	memcpy(reply.data.fs.name, t->fs_path.current_dir_name,
		   strlen(t->fs_path.current_dir_name) + 1);

	kernel::task::send_message(m.sender, reply);
}

} // namespace kernel::fs::fat
