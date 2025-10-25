/**
 * @file internal_common.hpp
 * @brief FAT32 internal shared declarations (not for external use)
 */

#pragma once

#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <map>
#include <queue>
#include <string>
#include "fat.hpp"

namespace kernel::fs::fat
{

// Global variables shared across FAT implementation
extern kernel::fs::BiosParameterBlock* VOLUME_BPB;
extern unsigned long BYTES_PER_CLUSTER;
extern unsigned long ENTRIES_PER_CLUSTER;
extern uint32_t* FAT_TABLE;
extern unsigned int FAT_TABLE_SECTOR;
extern kernel::fs::DirectoryEntry* ROOT_DIR;
extern std::queue<Message> pending_messages;

// FAT table write optimization constants
constexpr size_t FAT_WRITE_CHUNK_SIZE = 65536; // 64KB chunks for batch writes

// Change directory request tracking
extern std::map<fs_id_t, ProcessId> change_dir_requests;
extern std::map<fs_id_t, std::string> change_dir_names;
extern std::map<fs_id_t, std::string> parent_dir_names;
extern fs_id_t next_change_dir_id;

// Common utility functions
std::vector<char*> parse_path(const char* path);
void read_dir_entry_name_raw(const kernel::fs::DirectoryEntry& entry, char* dest);
void read_dir_entry_name(const kernel::fs::DirectoryEntry& entry, char* dest);
bool entry_name_is_equal(const kernel::fs::DirectoryEntry& entry, const char* name);
unsigned int calc_start_sector(cluster_t cluster_id);
cluster_t next_cluster(cluster_t cluster_id);
kernel::fs::DirectoryEntry* find_dir_entry(kernel::fs::DirectoryEntry* parent_dir,
										   const char* name);
kernel::fs::DirectoryEntry* find_empty_dir_entry();
cluster_t extend_cluster_chain(cluster_t last_cluster, int num_clusters);
cluster_t allocate_cluster_chain(size_t num_clusters);
void free_cluster_chain(cluster_t start_cluster, cluster_t keep_until_cluster = 0);
size_t count_free_clusters();

// Communication with block device
void send_read_req_to_blk_device(unsigned int sector,
								 size_t len,
								 MsgType dst_type,
								 fs_id_t request_id = 0,
								 size_t sequence = 0);

void send_write_req_to_blk_device(void* buffer,
								  unsigned int sector,
								  size_t len,
								  MsgType dst_type,
								  fs_id_t request_id = 0,
								  size_t sequence = 0);

// FAT table persistence
void write_fat_table_to_disk();

// Send file data to requester
void send_file_data(fs_id_t id,
					void* buf,
					size_t size,
					ProcessId requester,
					MsgType type,
					bool for_user);

// Directory entry persistence
void update_directory_entry_on_disk(kernel::fs::DirectoryEntry* entry,
									const char* name);

// Message handlers declarations
void handle_initialize(const Message& m);
void handle_get_file_info(const Message& m);
void handle_read_file_data(const Message& m);
void handle_get_directory_contents(const Message& m);
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
