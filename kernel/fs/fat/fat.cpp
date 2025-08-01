/**
 * @file fat.cpp
 * @brief FAT32 task main entry point
 */

#include "fat.hpp"
#include "fs/file_info.hpp"
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
	kernel::task::Task* t = kernel::task::CURRENT_TASK;
	t->is_initilized = false;
	pending_messages = std::queue<Message>();

	init_read_contexts();

	send_read_req_to_blk_device(
	    BOOT_SECTOR, kernel::hw::virtio::SECTOR_SIZE, MsgType::INITIALIZE_TASK);

	// Register message handlers
	t->add_msg_handler(MsgType::INITIALIZE_TASK, handle_initialize);
	t->add_msg_handler(MsgType::IPC_GET_FILE_INFO, handle_get_file_info);
	t->add_msg_handler(MsgType::IPC_READ_FILE_DATA, handle_read_file_data);
	t->add_msg_handler(MsgType::GET_DIRECTORY_CONTENTS, handle_get_directory_contents);
	t->add_msg_handler(MsgType::FS_OPEN, handle_fs_open);
	t->add_msg_handler(MsgType::FS_READ, handle_fs_read);
	t->add_msg_handler(MsgType::FS_WRITE, handle_fs_write);
	t->add_msg_handler(MsgType::FS_CLOSE, handle_fs_close);
	t->add_msg_handler(MsgType::FS_MKFILE, handle_fs_mkfile);
	t->add_msg_handler(MsgType::FS_REGISTER_PATH, handle_fs_register_path);
	t->add_msg_handler(MsgType::FS_PWD, handle_fs_pwd);
	t->add_msg_handler(MsgType::FS_CHANGE_DIR, handle_fs_change_dir);
	t->add_msg_handler(MsgType::FS_DUP2, handle_fs_dup2);

	kernel::task::process_messages(t);
}

}  // namespace kernel::fs::fat

// Export for backward compatibility
namespace kernel::fs
{
void fat32_service()
{
	kernel::fs::fat::fat32_service();
}
}  // namespace kernel::fs
