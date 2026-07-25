/**
 * @file internal_common.hpp
 * @brief FAT32 internal shared declarations (not for external use)
 */

#pragma once

#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "fat.hpp"
#include "memory/slab.hpp"

namespace kernel::fs::fat
{

// Global variables shared across FAT implementation
extern kernel::fs::BiosParameterBlock* VOLUME_BPB;
extern unsigned long BYTES_PER_CLUSTER;
extern unsigned long ENTRIES_PER_CLUSTER;
extern uint32_t* FAT_TABLE;
extern unsigned int FAT_TABLE_SECTOR;
extern kernel::fs::DirectoryEntry* ROOT_DIR;

// FAT table write optimization constants
constexpr size_t FAT_WRITE_CHUNK_SIZE = 65536; // 64KB chunks for batch writes

/// Space character used to right-pad the 8.3 short file name fields.
constexpr char SFN_PAD = 0x20;

/**
 * @brief One open file in the FS-owned ledger (issue #315 3b-8)
 *
 * Keyed by an opaque handle stored in the client's routing entry; the
 * offset lives HERE, so routing-entry copies (fork, dup2) share it —
 * POSIX open-file-description semantics.
 */
struct OpenFileRecord {
	uint32_t handle; ///< 0 = slot free
	ProcessId owner; ///< Creator; used only by the dead-owner sweep
	kernel::fs::DirectoryEntry* entry;
	size_t offset;
	int refcount; ///< dup2 adds a reference, close drops one
	char name[13];
};

constexpr int MAX_OPEN_FILES = 64;

/**
 * @brief One client's working directory, owned by the FS (issue #315 3b-9)
 *
 * Replaces Task::fs_path: the cwd used to live in the client's Task and
 * this module mutated it across the task boundary. Only root tasks own a
 * record; forked children act on their parent's (effective owner).
 */
struct ClientCwd {
	/// Slot occupancy. The kernel never runs static constructors, so a
	/// global table only gets zero-initialized: the free-slot sentinel MUST
	/// be all-zeroes (false), never a nonzero value like ProcessId INVALID
	/// (-1). Checking owner == INVALID here made every slot look owned by
	/// pid 0 (KERNEL, alive forever) and the table was born full.
	bool in_use;
	ProcessId owner; ///< Valid only while in_use
	/// Current directory cluster; owned buffer unless it is the shared
	/// ROOT_DIR (freed on replace / reset / sweep)
	kernel::fs::DirectoryEntry* dir;
	char dir_name[13];	 ///< Display name of the current directory
	char full_path[256]; ///< Absolute path from the root
};

constexpr int MAX_CWD_CLIENTS = 16;

/// Register (or reset to root) owner's working directory.
/// @return OK, or ERR_NO_MEMORY when the table is full even after sweeping
error_t cwd_register(ProcessId owner);
/// @return owner's cwd record, or nullptr when none is registered
ClientCwd* cwd_lookup(ProcessId owner);

/// @return The new handle, or 0 when the ledger is full even after sweeping
uint32_t open_file_create(kernel::fs::DirectoryEntry* entry,
						  const char* name,
						  ProcessId owner);
OpenFileRecord* open_file_get(uint32_t handle);
void open_file_addref(uint32_t handle);
void open_file_release(uint32_t handle);

// Common utility functions
void read_dir_entry_name_raw(const kernel::fs::DirectoryEntry& entry, char* dest);
void read_dir_entry_name(const kernel::fs::DirectoryEntry& entry, char* dest);
bool entry_name_is_equal(const kernel::fs::DirectoryEntry& entry, const char* name);
unsigned int calc_start_sector(cluster_t cluster_id);
cluster_t next_cluster(cluster_t cluster_id);
kernel::fs::DirectoryEntry* find_dir_entry(kernel::fs::DirectoryEntry* parent_dir,
										   const char* name);
kernel::fs::DirectoryEntry* find_empty_dir_entry();

/**
 * @brief Iterate the live entries of a single directory cluster
 *
 * Single scan primitive for directory clusters: visits slots in order until
 * the end marker, skipping deleted and long-file-name slots. fn receives a
 * DirectoryEntry& and returns true to stop at that entry.
 *
 * @return The entry fn stopped at, or nullptr when the scan reached the end
 * marker (or the cluster was exhausted)
 */
template<typename Fn>
kernel::fs::DirectoryEntry* scan_valid_entries(kernel::fs::DirectoryEntry* dir,
											   Fn fn)
{
	for (unsigned long i = 0; i < ENTRIES_PER_CLUSTER; ++i) {
		if (dir[i].name[0] == DIR_ENTRY_END) {
			return nullptr;
		}

		if (dir[i].name[0] == DIR_ENTRY_DELETED ||
			dir[i].attribute == kernel::fs::entry_attribute::LONG_NAME) {
			continue;
		}

		if (fn(dir[i])) {
			return &dir[i];
		}
	}

	return nullptr;
}
// The (fat, total_clusters) overloads operate on an explicit FAT so tests
// can use a private table; the parameterless-FAT overloads wrap the live
// globals. Tests must never swap the globals themselves: the FS task uses
// them concurrently (issue #313 regression).
cluster_t extend_cluster_chain(uint32_t* fat,
							   size_t total_clusters,
							   cluster_t last_cluster,
							   int num_clusters);
cluster_t extend_cluster_chain(cluster_t last_cluster, int num_clusters);
cluster_t allocate_cluster_chain(uint32_t* fat,
								 size_t total_clusters,
								 size_t num_clusters);
cluster_t allocate_cluster_chain(size_t num_clusters);
void free_cluster_chain(uint32_t* fat, cluster_t start_cluster);
void free_cluster_chain(cluster_t start_cluster);
size_t count_free_clusters(const uint32_t* fat, size_t total_clusters);
size_t count_free_clusters();
size_t total_fat_clusters();

/**
 * @brief Synchronously read len bytes starting at sector from the block
 * device (call/reply RPC, issue #314 Stage B)
 * @return Owning buffer, or empty on delivery/device failure (logged)
 */
kernel::memory::unique_kbuf<> read_from_blk(unsigned int sector, size_t len);

/**
 * @brief Synchronously write len bytes to the block device
 * @param buffer Data to write; ownership moves to the blk task, which
 * frees it once the device write completes (success or failure)
 * @return OK, or the delivery/device error
 */
error_t write_to_blk(void* buffer, unsigned int sector, size_t len);

// Synchronous FAT32 initialization: BPB -> FAT -> root directory
error_t initialize_fat32();

// FAT table persistence
void write_fat_table_to_disk();

/**
 * @brief Reply to a read-style request with file data
 *
 * Copies the data into a fresh OOL buffer whose ownership moves with the
 * reply: a user requester gets it mapped at the syscall boundary (released
 * via ool_release), a kernel requester frees it directly.
 */
void reply_file_data(const Message& req, const void* buf, size_t size);

/**
 * @brief Reply with an error result and no payload
 */
void reply_error(const Message& req, error_t result);

// Directory entry persistence
void persist_directory_entry(kernel::fs::DirectoryEntry* entry, const char* name);

// Message handlers declarations
void handle_fs_stat(const Message& m);
void handle_fs_load(const Message& m);
void handle_fs_list_dir(const Message& m);
void handle_fs_open(const Message& m);
void handle_fs_read(const Message& m);
void handle_fs_write(const Message& m);
void handle_fs_close(const Message& m);
void handle_fs_mkfile(const Message& m);
void handle_fs_register_path(const Message& m);
void handle_fs_pwd(const Message& m);
void handle_fs_change_dir(const Message& m);
void handle_fs_dup2(const Message& m);

} // namespace kernel::fs::fat
