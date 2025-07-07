/**
 * @file internal_common.hpp
 * @brief FAT32 internal shared declarations (not for external use)
 */

#pragma once

#include <cstdint>
#include <map>
#include <queue>
#include <string>

#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

#include "../fat.hpp"

namespace kernel::fs::fat
{

// Global variables shared across FAT implementation
extern kernel::fs::bios_parameter_block* VOLUME_BPB;
extern unsigned long BYTES_PER_CLUSTER;
extern unsigned long ENTRIES_PER_CLUSTER;
extern uint32_t* FAT_TABLE;
extern unsigned int FAT_TABLE_SECTOR;
extern kernel::fs::directory_entry* ROOT_DIR;
extern std::queue<message> pending_messages;

// Change directory request tracking
extern std::map<fs_id_t, ProcessId> change_dir_requests;
extern std::map<fs_id_t, std::string> change_dir_names;
extern std::map<fs_id_t, std::string> parent_dir_names;
extern fs_id_t next_change_dir_id;

// Common utility functions
std::vector<char*> parse_path(const char* path);
void read_dir_entry_name(const kernel::fs::directory_entry& entry, char* dest);
bool entry_name_is_equal(const kernel::fs::directory_entry& entry, const char* name);
unsigned int calc_start_sector(cluster_t cluster_id);
cluster_t next_cluster(cluster_t cluster_id);
kernel::fs::directory_entry*
find_dir_entry(kernel::fs::directory_entry* parent_dir, const char* name);
kernel::fs::directory_entry* find_empty_dir_entry();
cluster_t extend_cluster_chain(cluster_t last_cluster, int num_clusters);
cluster_t allocate_cluster_chain(size_t num_clusters);

// Communication with block device
void send_read_req_to_blk_device(unsigned int sector,
								 size_t len,
								 msg_t dst_type,
								 fs_id_t request_id = 0,
								 size_t sequence = 0);

// Send file data to requester
void send_file_data(fs_id_t id,
					void* buf,
					size_t size,
					ProcessId requester,
					msg_t type,
					bool for_user);

// Message handlers declarations
void handle_initialize(const message& m);
void handle_get_file_info(const message& m);
void handle_read_file_data(const message& m);
void handle_get_directory_contents(const message& m);
void handle_fs_open(const message& m);
void handle_fs_read(const message& m);
void handle_fs_close(const message& m);
void handle_fs_mkfile(const message& m);
void handle_fs_register_path(const message& m);
void handle_fs_pwd(const message& m);
void handle_fs_change_dir(const message& m);

} // namespace kernel::fs::fat
