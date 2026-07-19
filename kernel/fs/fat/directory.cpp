/**
 * @file directory.cpp
 * @brief FAT32 directory operations implementation
 */

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/stat.hpp>
#include <libs/common/types.hpp>
#include <string>
#include <vector>
#include "fat.hpp"
#include "fs/path.hpp"
#include "graphics/font.hpp"
#include "internal_common.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace kernel::fs::fat
{

namespace
{

/// Set fs_path (full_path / current_dir_name) from an already-known parent
/// path string; used for ".." when the parent's path was precomputed.
void set_fs_path_to_parent(kernel::task::Task* t, const std::string& parent_path)
{
	strncpy(t->fs_path.full_path, parent_path.c_str(),
			sizeof(t->fs_path.full_path) - 1);
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

/// Set fs_path (full_path / current_dir_name) by appending a child directory
/// name; used when the current directory changes to a named subdirectory.
void set_fs_path_to_child(kernel::task::Task* t, const std::string& dir_name)
{
	strncpy(t->fs_path.current_dir_name, dir_name.c_str(), 12);
	t->fs_path.current_dir_name[12] = '\0';

	if (strcmp(t->fs_path.full_path, "/") == 0) {
		snprintf(t->fs_path.full_path, sizeof(t->fs_path.full_path), "/%s",
				 dir_name.c_str());
	} else {
		const size_t len = strlen(t->fs_path.full_path);
		snprintf(t->fs_path.full_path + len, sizeof(t->fs_path.full_path) - len,
				 "/%s", dir_name.c_str());
	}
}

/// Reset fs_path (current_dir / current_dir_name / full_path) to the root
/// directory. State only: this does not send a reply.
void set_fs_path_to_root(kernel::task::Task* t)
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

/// Replace fs_path.current_dir with a newly loaded directory-cluster buffer,
/// freeing the previous buffer (unless it is the shared ROOT_DIR).
void set_fs_path_current_dir(kernel::task::Task* t, DirectoryEntry* new_dir)
{
	if (t->fs_path.current_dir != nullptr && t->fs_path.current_dir != ROOT_DIR) {
		kernel::memory::free(t->fs_path.current_dir);
	}
	t->fs_path.current_dir = new_dir;
}

/// Reply to an FS_CHANGE_DIR request with the (possibly truncated) new
/// directory name and OK.
void reply_change_dir(const Message& m, const char* name)
{
	Message reply = { .type = MsgType::FS_CHANGE_DIR,
					  .sender = process_ids::FS_FAT32 };
	// Truncate to the reply buffer: full_path can be far longer than
	// data.fs.name and an unchecked memcpy corrupts the Message (issue #313).
	strncpy(reply.data.fs.name, name, sizeof(reply.data.fs.name) - 1);
	reply.data.fs.name[sizeof(reply.data.fs.name) - 1] = '\0';
	reply.result = OK;
	kernel::task::reply(m, &reply);
}

/// Reset to the root directory and reply to the FS_CHANGE_DIR request.
void handle_change_to_root(const Message& m, kernel::task::Task* t)
{
	set_fs_path_to_root(t);
	reply_change_dir(m, "/");
}

void send_error_response(const Message& m, const char* error_msg = nullptr)
{
	Message reply = { .type = MsgType::FS_CHANGE_DIR,
					  .sender = process_ids::FS_FAT32 };
	reply.result = ERR_NO_FILE;
	if (error_msg != nullptr) {
		strncpy(reply.data.fs.name, error_msg, sizeof(reply.data.fs.name) - 1);
		reply.data.fs.name[sizeof(reply.data.fs.name) - 1] = '\0';
	}
	kernel::task::reply(m, &reply);
}

void handle_parent_directory_change(const Message& m, kernel::task::Task* t)
{
	if (t->fs_path.current_dir == ROOT_DIR) {
		handle_change_to_root(m, t);
		return;
	}

	DirectoryEntry* parent_entry = find_dir_entry(t->fs_path.current_dir, "..");
	if (parent_entry == nullptr) {
		send_error_response(m);
		return;
	}

	if (parent_entry->first_cluster() == 0) {
		handle_change_to_root(m, t);
		return;
	}

	const std::string current_full_path = t->fs_path.full_path;
	const size_t last_slash = current_full_path.rfind('/');
	const std::string parent_path =
			(last_slash == 0) ? "/" : current_full_path.substr(0, last_slash);

	// Synchronous cluster load (issue #314 Stage B): the reply below
	// reports the real outcome, and the change_dir_requests/names maps
	// that tracked the in-flight load are gone.
	auto buf = read_from_blk(calc_start_sector(parent_entry->first_cluster()),
							 BYTES_PER_CLUSTER);
	if (!buf) {
		// Device read failed: keep the current directory instead of
		// pointing it at a null buffer.
		send_error_response(m);
		return;
	}

	set_fs_path_current_dir(t, reinterpret_cast<DirectoryEntry*>(buf.release()));
	set_fs_path_to_parent(t, parent_path);
	reply_change_dir(m, parent_path.c_str());
}

void handle_normal_directory_change(const Message& m,
									kernel::task::Task* t,
									const char* path_name)
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

	auto buf = read_from_blk(calc_start_sector(entry->first_cluster()),
							 BYTES_PER_CLUSTER);
	if (!buf) {
		send_error_response(m);
		return;
	}

	set_fs_path_current_dir(t, reinterpret_cast<DirectoryEntry*>(buf.release()));
	set_fs_path_to_child(t, upper_name);
	reply_change_dir(m, upper_name);
}

kernel::task::Task* get_effective_task(ProcessId sender)
{
	auto* t = kernel::task::get_task(sender);
	if (t != nullptr && t->parent_id != process_ids::INVALID) {
		return kernel::task::get_task(t->parent_id);
	}
	return t;
}

} // namespace

void handle_fs_change_dir(const Message& m)
{
	auto* task = get_effective_task(m.sender);
	if (task == nullptr) {
		send_error_response(m, "Task not found");
		return;
	}

	const char* path_name = reinterpret_cast<const char*>(m.data.fs.name);

	if (strcmp(path_name, "/") == 0) {
		handle_change_to_root(m, task);
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
	auto* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		// Requester is gone; there is no one to reply to.
		LOG_ERROR("Task %d not found", m.sender.raw());
		return;
	}

	if (t->parent_id != process_ids::INVALID) {
		kernel::task::Task* parent = kernel::task::get_task(t->parent_id);
		if (parent != nullptr) {
			t = parent;
		}
	}

	DirectoryEntry* current_dir = t->fs_path.current_dir;
	if (current_dir == nullptr) {
		LOG_ERROR("current directory not set");
		reply_error(m, ERR_NO_FILE);
		return;
	}
	std::vector<char> entries(ENTRIES_PER_CLUSTER * sizeof(DirectoryEntry));
	int entries_count = 0;
	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (current_dir[i].name[0] == DIR_ENTRY_END) {
			break;
		}

		if (current_dir[i].name[0] == DIR_ENTRY_DELETED) {
			continue;
		}

		if (current_dir[i].attribute == entry_attribute::LONG_NAME) {
			continue;
		}

		memcpy(&entries[entries_count * sizeof(DirectoryEntry)], &current_dir[i],
			   sizeof(DirectoryEntry));
		++entries_count;
	}

	void* buf = nullptr;
	if (entries_count > 0) {
		buf = kernel::memory::alloc(entries_count * sizeof(Stat),
									kernel::memory::ALLOC_ZEROED);
		if (buf == nullptr) {
			// Reply with an empty listing so the requester is not left
			// blocking forever.
			LOG_ERROR("failed to allocate stat buffer");
			entries_count = 0;
		}
	}

	for (int i = 0; i < entries_count; ++i) {
		Stat* s = reinterpret_cast<Stat*>(buf) + i;
		const DirectoryEntry* e = reinterpret_cast<const DirectoryEntry*>(
				&entries[i * sizeof(DirectoryEntry)]);
		read_dir_entry_name(*e, s->name);
		s->size = e->file_size;
		s->type = e->attribute == entry_attribute::DIRECTORY
						  ? StatType::DIRECTORY
						  : StatType::REGULAR_FILE;
	}

	Message sm = { .type = MsgType::GET_DIRECTORY_CONTENTS,
				   .sender = process_ids::FS_FAT32 };
	sm.result = OK;
	sm.tool_desc.addr = buf;
	sm.tool_desc.size = entries_count * sizeof(Stat);
	sm.tool_desc.present = buf != nullptr;

	kernel::task::reply(m, &sm);
}

void handle_fs_pwd(const Message& m)
{
	Message reply = { .type = MsgType::FS_PWD, .sender = process_ids::FS_FAT32 };

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		// Requester is gone; there is no one to reply to.
		LOG_ERROR("Task %d not found", m.sender.raw());
		return;
	}

	if (t->parent_id != process_ids::INVALID) {
		kernel::task::Task* parent = kernel::task::get_task(t->parent_id);
		if (parent != nullptr) {
			t = parent;
		}
	}

	if (t->fs_path.current_dir == nullptr) {
		reply_error(m, ERR_NO_FILE);
		return;
	}

	strncpy(reply.data.fs.name, t->fs_path.current_dir_name,
			sizeof(reply.data.fs.name) - 1);
	reply.data.fs.name[sizeof(reply.data.fs.name) - 1] = '\0';
	reply.result = OK;

	kernel::task::reply(m, &reply);
}

void persist_directory_entry(DirectoryEntry* entry, const char* name)
{
	if (entry == nullptr || ROOT_DIR == nullptr) {
		LOG_ERROR("Invalid directory entry or ROOT_DIR not initialized");
		return;
	}

	DirectoryEntry* disk_entry = nullptr;
	for (int i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (ROOT_DIR[i].name[0] == DIR_ENTRY_END) {
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

	// Write the entire ROOT_DIR cluster back to disk; write_to_blk hands
	// buffer ownership to the blk task and reports the real outcome
	const error_t err =
			write_to_blk(write_buffer, root_dir_sector, BYTES_PER_CLUSTER);
	if (IS_ERR(err)) {
		LOG_ERROR("failed to persist directory entry: %d", err);
	}
}

} // namespace kernel::fs::fat
