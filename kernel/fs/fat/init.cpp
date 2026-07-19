/**
 * @file init.cpp
 * @brief FAT32 initialization implementation
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include "fat.hpp"
#include "fs/path.hpp"
#include "internal_common.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace kernel::fs::fat
{

kernel::fs::BiosParameterBlock* VOLUME_BPB = nullptr;
unsigned long BYTES_PER_CLUSTER = 0;
unsigned long ENTRIES_PER_CLUSTER = 0;
uint32_t* FAT_TABLE = nullptr;
unsigned int FAT_TABLE_SECTOR = 0;
kernel::fs::DirectoryEntry* ROOT_DIR = nullptr;

error_t initialize_fat32()
{
	// Straight-line synchronous init (issue #314 Stage B): BPB -> FAT ->
	// root directory, each a call to the blk service. The old async
	// bootstrap (INITIALIZE_TASK-tagged replies, pending_messages backlog
	// and its queue re-injection) is gone; requests that arrive while this
	// runs simply wait in the ring until process_messages starts.
	auto bpb_buf = read_from_blk(BOOT_SECTOR, SECTOR_SIZE);
	if (!bpb_buf) {
		return ERR_FAILED_READ_FROM_DEVICE;
	}
	VOLUME_BPB =
			reinterpret_cast<kernel::fs::BiosParameterBlock*>(bpb_buf.release());

	BYTES_PER_CLUSTER = static_cast<unsigned long>(VOLUME_BPB->bytes_per_sector) *
						VOLUME_BPB->sectors_per_cluster;
	ENTRIES_PER_CLUSTER = BYTES_PER_CLUSTER / sizeof(kernel::fs::DirectoryEntry);
	FAT_TABLE_SECTOR = VOLUME_BPB->reserved_sector_count;

	const size_t table_size = static_cast<size_t>(VOLUME_BPB->fat_size_32) *
							  static_cast<size_t>(SECTOR_SIZE);
	auto fat_buf = read_from_blk(FAT_TABLE_SECTOR, table_size);
	if (!fat_buf) {
		return ERR_FAILED_READ_FROM_DEVICE;
	}
	FAT_TABLE = reinterpret_cast<uint32_t*>(fat_buf.release());

	auto root_buf = read_from_blk(calc_start_sector(VOLUME_BPB->root_cluster),
								  BYTES_PER_CLUSTER);
	if (!root_buf) {
		return ERR_FAILED_READ_FROM_DEVICE;
	}
	ROOT_DIR = reinterpret_cast<kernel::fs::DirectoryEntry*>(root_buf.release());

	return OK;
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

	// Root tasks start at the root directory. The FS runs in ring 0, so it
	// sets the task's fs_path directly; the old forward-to-KERNEL hop (and
	// its heap-allocated Path hand-off) is gone (issue #314 Stage B).
	if (t->parent_id == process_ids::INVALID) {
		t->fs_path = init_path(ROOT_DIR);
	}

	reply.result = OK;
	kernel::task::reply(m, &reply);
}

} // namespace kernel::fs::fat
