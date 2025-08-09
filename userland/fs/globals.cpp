/**
 * @file globals.cpp
 * @brief Global variables implementation for FAT32 userland filesystem
 */

#include "globals.hpp"

namespace userland::fs
{

// Global variables shared across FAT implementation
BiosParameterBlock* VOLUME_BPB = nullptr;
unsigned long BYTES_PER_CLUSTER = 0;
unsigned long ENTRIES_PER_CLUSTER = 0;
uint32_t* FAT_TABLE = nullptr;
unsigned int FAT_TABLE_SECTOR = 0;
DirectoryEntry* ROOT_DIR = nullptr;
std::queue<Message> pending_messages;

// Change directory request tracking
std::map<fs_id_t, ProcessId> change_dir_requests;
std::map<fs_id_t, std::string> change_dir_names;
std::map<fs_id_t, std::string> parent_dir_names;
fs_id_t next_change_dir_id = 0;

}  // namespace userland::fs