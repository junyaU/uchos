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
#include <utility>
#include "fat.hpp"
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

/// FS-owned working-directory table (issue #315 3b-9): the cwd used to
/// live in each client's Task (fs_path), which this file mutated across
/// the task boundary in ~40 places. Now it is FS state, keyed by owner.
ClientCwd cwd_table[MAX_CWD_CLIENTS];

/// Drop a record's owned directory buffer (the shared ROOT_DIR is not ours)
void free_cwd_dir(ClientCwd& c)
{
	if (c.dir != nullptr && c.dir != ROOT_DIR) {
		kernel::memory::free(c.dir);
	}
	c.dir = nullptr;
}

/// Reclaim records whose owner task no longer exists (same pattern as the
/// open-file ledger: process exit does not notify the FS by design)
void sweep_dead_cwd_owners()
{
	for (auto& c : cwd_table) {
		if (c.owner != process_ids::INVALID &&
			kernel::task::get_task(c.owner) == nullptr) {
			free_cwd_dir(c);
			c.owner = process_ids::INVALID;
		}
	}
}

/// Set the record's name fields from an already-known parent path string;
/// used for ".." when the parent's path was precomputed.
void set_cwd_to_parent(ClientCwd& c, const std::string& parent_path)
{
	strncpy(c.full_path, parent_path.c_str(), sizeof(c.full_path) - 1);
	c.full_path[sizeof(c.full_path) - 1] = '\0';

	if (parent_path == "/") {
		c.dir_name[0] = '/';
		c.dir_name[1] = '\0';
		return;
	}

	const size_t last_slash = parent_path.rfind('/');
	if (last_slash != std::string::npos) {
		const std::string dir_name = parent_path.substr(last_slash + 1);
		strncpy(c.dir_name, dir_name.c_str(), 12);
		c.dir_name[12] = '\0';
	}
}

/// Set the record's name fields by appending a child directory name; used
/// when the current directory changes to a named subdirectory.
void set_cwd_to_child(ClientCwd& c, const std::string& dir_name)
{
	strncpy(c.dir_name, dir_name.c_str(), 12);
	c.dir_name[12] = '\0';

	if (strcmp(c.full_path, "/") == 0) {
		snprintf(c.full_path, sizeof(c.full_path), "/%s", dir_name.c_str());
	} else {
		const size_t len = strlen(c.full_path);
		snprintf(c.full_path + len, sizeof(c.full_path) - len, "/%s",
				 dir_name.c_str());
	}
}

/// Reset a record to the root directory. State only: no reply is sent.
void set_cwd_to_root(ClientCwd& c)
{
	free_cwd_dir(c);
	c.dir = ROOT_DIR;
	c.dir_name[0] = '/';
	c.dir_name[1] = '\0';
	strncpy(c.full_path, "/", sizeof(c.full_path) - 1);
	c.full_path[sizeof(c.full_path) - 1] = '\0';
}

/// Replace the record's directory with a newly loaded cluster buffer,
/// freeing the previous one (unless it is the shared ROOT_DIR).
void set_cwd_dir(ClientCwd& c, DirectoryEntry* new_dir)
{
	free_cwd_dir(c);
	c.dir = new_dir;
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
void handle_change_to_root(const Message& m, ClientCwd& c)
{
	set_cwd_to_root(c);
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

void handle_parent_directory_change(const Message& m, ClientCwd& c)
{
	if (c.dir == ROOT_DIR) {
		handle_change_to_root(m, c);
		return;
	}

	DirectoryEntry* parent_entry = find_dir_entry(c.dir, "..");
	if (parent_entry == nullptr) {
		send_error_response(m);
		return;
	}

	if (parent_entry->first_cluster() == 0) {
		handle_change_to_root(m, c);
		return;
	}

	const std::string current_full_path = c.full_path;
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

	set_cwd_dir(c, reinterpret_cast<DirectoryEntry*>(buf.release()));
	set_cwd_to_parent(c, parent_path);
	reply_change_dir(m, parent_path.c_str());
}

void handle_normal_directory_change(const Message& m,
									ClientCwd& c,
									const char* path_name)
{
	char upper_name[256];
	strncpy(upper_name, path_name, sizeof(upper_name) - 1);
	upper_name[sizeof(upper_name) - 1] = '\0';
	kernel::graphics::to_upper(upper_name);

	DirectoryEntry* entry = find_dir_entry(c.dir, upper_name);
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

	set_cwd_dir(c, reinterpret_cast<DirectoryEntry*>(buf.release()));
	set_cwd_to_child(c, upper_name);
	reply_change_dir(m, upper_name);
}

/// Resolve the owner whose working directory an FS request acts on: a
/// forked child acts on its parent's cwd while the parent is alive. This
/// is a read-only query of kernel task info; when the FS leaves ring 0
/// (3b-10) the effective owner becomes part of the request context.
ProcessId effective_cwd_owner(ProcessId sender)
{
	auto* t = kernel::task::get_task(sender);
	if (t == nullptr || t->parent_id == process_ids::INVALID) {
		return sender;
	}

	return kernel::task::get_task(t->parent_id) != nullptr ? t->parent_id : sender;
}

} // namespace

error_t cwd_register(ProcessId owner)
{
	// Re-registration resets the existing record instead of duplicating it
	if (ClientCwd* existing = cwd_lookup(owner); existing != nullptr) {
		set_cwd_to_root(*existing);
		return OK;
	}

	for (int attempt = 0; attempt < 2; ++attempt) {
		for (auto& c : cwd_table) {
			if (c.owner != process_ids::INVALID) {
				continue;
			}

			c.owner = owner;
			c.dir = nullptr;
			set_cwd_to_root(c);
			return OK;
		}

		sweep_dead_cwd_owners();
	}

	LOG_ERROR("cwd table full (owner %d)", owner.raw());
	return ERR_NO_MEMORY;
}

ClientCwd* cwd_lookup(ProcessId owner)
{
	if (owner == process_ids::INVALID) {
		return nullptr;
	}

	for (auto& c : cwd_table) {
		if (c.owner == owner) {
			return &c;
		}
	}

	return nullptr;
}

void handle_fs_register_path(const Message& m)
{
	Message reply = { .type = MsgType::FS_REGISTER_PATH,
					  .sender = process_ids::FS_FAT32 };

	kernel::task::Task* t = kernel::task::get_task(m.sender);
	if (t == nullptr) {
		LOG_ERROR("Task %d not found", m.sender.raw());
		return;
	}

	// Only root tasks own a working directory; a forked child acts on its
	// parent's (effective_cwd_owner), so it registers nothing
	reply.result =
			t->parent_id == process_ids::INVALID ? cwd_register(m.sender) : OK;
	kernel::task::reply(m, &reply);
}

void handle_fs_change_dir(const Message& m)
{
	ClientCwd* c = cwd_lookup(effective_cwd_owner(m.sender));
	if (c == nullptr) {
		send_error_response(m, "no working directory registered");
		return;
	}

	const char* path_name = reinterpret_cast<const char*>(m.data.fs.name);

	if (strcmp(path_name, "/") == 0) {
		handle_change_to_root(m, *c);
		return;
	}

	if (strcmp(path_name, "..") == 0) {
		handle_parent_directory_change(m, *c);
		return;
	}

	handle_normal_directory_change(m, *c, path_name);
}

void handle_fs_list_dir(const Message& m)
{
	ClientCwd* c = cwd_lookup(effective_cwd_owner(m.sender));
	if (c == nullptr || c->dir == nullptr) {
		LOG_ERROR("no working directory for task %d", m.sender.raw());
		reply_error(m, ERR_NO_FILE);
		return;
	}

	DirectoryEntry* current_dir = c->dir;

	// User-destined OOL: sized for the worst case up front, the real entry
	// count only shrinks the payload (ool.size) below
	auto stat_buf =
			kernel::task::make_ool_buffer(ENTRIES_PER_CLUSTER * sizeof(Stat));
	if (!stat_buf) {
		LOG_ERROR("failed to allocate stat buffer");
		reply_error(m, ERR_NO_MEMORY);
		return;
	}

	int entries_count = 0;
	scan_valid_entries(current_dir, [&](const DirectoryEntry& entry) {
		Stat* s = reinterpret_cast<Stat*>(stat_buf.get()) + entries_count;
		read_dir_entry_name(entry, s->name);
		s->size = entry.file_size;
		s->type = entry.attribute == entry_attribute::DIRECTORY
						  ? StatType::DIRECTORY
						  : StatType::REGULAR_FILE;
		++entries_count;
		return false;
	});

	Message sm = { .type = MsgType::FS_LIST_DIR, .sender = process_ids::FS_FAT32 };
	sm.result = OK;

	// An empty listing replies without a payload (size 0 frees the buffer)
	kernel::task::reply_with_ool(m, &sm, std::move(stat_buf),
								 entries_count * sizeof(Stat));
}

void handle_fs_pwd(const Message& m)
{
	Message reply = { .type = MsgType::FS_PWD, .sender = process_ids::FS_FAT32 };

	ClientCwd* c = cwd_lookup(effective_cwd_owner(m.sender));
	if (c == nullptr || c->dir == nullptr) {
		reply_error(m, ERR_NO_FILE);
		return;
	}

	strncpy(reply.data.fs.name, c->dir_name, sizeof(reply.data.fs.name) - 1);
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

	DirectoryEntry* disk_entry = find_dir_entry(ROOT_DIR, name);
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
