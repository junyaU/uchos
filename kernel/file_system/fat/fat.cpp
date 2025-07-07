/**
 * @file fat.cpp
 * @brief FAT32 task main entry point
 */

#include "file_system/fat.hpp"
#include "file_system/file_descriptor.hpp"
#include "file_system/file_info.hpp"
#include "hardware/virtio/blk.hpp"
#include "internal_common.hpp"
#include "task/task.hpp"
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <queue>

namespace kernel::fs::fat
{

void fat32_service()
{
	kernel::task::task* t = kernel::task::CURRENT_TASK;
	t->is_initilized = false;
	pending_messages = std::queue<message>();

	init_read_contexts();
	init_fds();

	send_read_req_to_blk_device(BOOT_SECTOR, kernel::hw::virtio::SECTOR_SIZE,
								msg_t::INITIALIZE_TASK);

	// Register message handlers
	t->add_msg_handler(msg_t::INITIALIZE_TASK, handle_initialize);
	t->add_msg_handler(msg_t::IPC_GET_FILE_INFO, handle_get_file_info);
	t->add_msg_handler(msg_t::IPC_READ_FILE_DATA, handle_read_file_data);
	t->add_msg_handler(msg_t::GET_DIRECTORY_CONTENTS, handle_get_directory_contents);
	t->add_msg_handler(msg_t::FS_OPEN, handle_fs_open);
	t->add_msg_handler(msg_t::FS_READ, handle_fs_read);
	t->add_msg_handler(msg_t::FS_CLOSE, handle_fs_close);
	t->add_msg_handler(msg_t::FS_MKFILE, handle_fs_mkfile);
	t->add_msg_handler(msg_t::FS_REGISTER_PATH, handle_fs_register_path);
	t->add_msg_handler(msg_t::FS_PWD, handle_fs_pwd);
	t->add_msg_handler(msg_t::FS_CHANGE_DIR, handle_fs_change_dir);

	kernel::task::process_messages(t);
}

} // namespace kernel::fs::fat

// Export for backward compatibility
namespace kernel::fs
{
void fat32_service() { kernel::fs::fat::fat32_service(); }
} // namespace kernel::fs
