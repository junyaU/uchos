/**
 * @file globals.hpp
 * @brief Global variables for FAT32 userland filesystem
 */

#pragma once

#include "common.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <map>
#include <queue>
#include <string>

namespace userland::fs
{

// Global variables shared across FAT implementation
extern BiosParameterBlock* VOLUME_BPB;
extern unsigned long BYTES_PER_CLUSTER;
extern unsigned long ENTRIES_PER_CLUSTER;
extern uint32_t* FAT_TABLE;
extern unsigned int FAT_TABLE_SECTOR;
extern DirectoryEntry* ROOT_DIR;
extern std::queue<Message> pending_messages;

// FAT table write optimization constants
constexpr size_t FAT_WRITE_CHUNK_SIZE = 65536;  // 64KB chunks for batch writes

// Change directory request tracking
extern std::map<fs_id_t, ProcessId> change_dir_requests;
extern std::map<fs_id_t, std::string> change_dir_names;
extern std::map<fs_id_t, std::string> parent_dir_names;
extern fs_id_t next_change_dir_id;

}  // namespace userland::fs