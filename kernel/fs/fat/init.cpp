/**
 * @file init.cpp
 * @brief FAT32 initialization implementation
 */

#include "fat.hpp"
#include "fs/path.hpp"
#include "hardware/virtio/blk.hpp"
#include "internal_common.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <queue>

namespace kernel::fs::fat
{

kernel::fs::BiosParameterBlock* VOLUME_BPB = nullptr;
unsigned long BYTES_PER_CLUSTER = 0;
unsigned long ENTRIES_PER_CLUSTER = 0;
uint32_t* FAT_TABLE = nullptr;
unsigned int FAT_TABLE_SECTOR = 0;
kernel::fs::DirectoryEntry* ROOT_DIR = nullptr;
std::queue<Message> pending_messages;

void handle_initialize(const Message& m)
{
	if (m.data.blk_io.sector == BOOT_SECTOR) {
		VOLUME_BPB = reinterpret_cast<kernel::fs::BiosParameterBlock*>(m.data.blk_io.buf);
		BYTES_PER_CLUSTER = static_cast<unsigned long>(VOLUME_BPB->bytes_per_sector) *
		                    VOLUME_BPB->sectors_per_cluster;
		ENTRIES_PER_CLUSTER = BYTES_PER_CLUSTER / sizeof(kernel::fs::DirectoryEntry);
		FAT_TABLE_SECTOR = VOLUME_BPB->reserved_sector_count;

		const size_t table_size = static_cast<size_t>(VOLUME_BPB->fat_size_32) *
		                          static_cast<size_t>(kernel::hw::virtio::SECTOR_SIZE);

		send_read_req_to_blk_device(FAT_TABLE_SECTOR, table_size, MsgType::INITIALIZE_TASK);
	} else if (m.data.blk_io.sector == FAT_TABLE_SECTOR) {
		FAT_TABLE = reinterpret_cast<uint32_t*>(m.data.blk_io.buf);
		const unsigned int root_cluster = VOLUME_BPB->root_cluster;

		send_read_req_to_blk_device(
		    calc_start_sector(root_cluster), BYTES_PER_CLUSTER, MsgType::INITIALIZE_TASK);
	} else {
		ROOT_DIR = reinterpret_cast<kernel::fs::DirectoryEntry*>(m.data.blk_io.buf);
		kernel::task::CURRENT_TASK->is_initilized = true;

		while (!pending_messages.empty()) {
			auto msg = pending_messages.front();
			pending_messages.pop();
			kernel::task::CURRENT_TASK->messages.push(msg);
		}
	}
}

void handle_fs_register_path(const Message& m)
{
	if (!kernel::task::CURRENT_TASK->is_initilized) {
		pending_messages.push(m);
		return;
	}

	Message reply = { .type = MsgType::FS_REGISTER_PATH, .sender = m.sender };

	void* buf;
	ALLOC_OR_RETURN(buf, sizeof(Path), kernel::memory::ALLOC_ZEROED);

	Path p = init_path(ROOT_DIR);
	memcpy(buf, &p, sizeof(Path));
	reply.data.fs.buf = buf;

	kernel::task::send_message(process_ids::KERNEL, reply);
}

}  // namespace kernel::fs::fat
