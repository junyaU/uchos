#pragma once

#include "types.hpp"
#include "process_id.hpp"
#include <cstddef>
#include <cstdint>

#define __user

// flags for sys_ipc
constexpr int IPC_RECV = 0;
constexpr int IPC_SEND = 1;

enum class msg_t : int32_t {
	NO_TASK = -1,
	NOTIFY_KEY_INPUT,
	NOTIFY_XHCI,
	NOTIFY_TIMER_TIMEOUT,
	NOTIFY_WRITE,
	INITIALIZE_TASK,
	IPC_TIME,
	IPC_EXIT_TASK,
	IPC_MEMORY_USAGE,
	IPC_PCI,
	IPC_WRITE_TO_BLK_DEVICE,
	IPC_READ_FROM_BLK_DEVICE,
	IPC_OOL_MEMORY_DEALLOC,
	GET_DIRECTORY_CONTENTS,
	IPC_GET_FILE_INFO,
	IPC_READ_FILE_DATA,
	IPC_RELEASE_FILE_BUFFER,
	FS_OPEN,
	FS_CLOSE,
	FS_READ,
	FS_WRITE,
	FS_MKFILE,
	FS_REGISTER_PATH,
	FS_GET_CWD,
	MAX_MESSAGE_TYPE, // must be the last
};

constexpr int32_t total_message_types =
		static_cast<int32_t>(msg_t::MAX_MESSAGE_TYPE);

enum class timeout_action_t : uint8_t {
	TERMINAL_CURSOR_BLINK,
	SWITCH_TASK,
};

struct msg_ool_desc_t {
	void __user* addr;
	size_t size;
	bool present = false;
};

struct message {
	msg_t type;
	ProcessId sender;
	msg_ool_desc_t tool_desc;
	bool is_end_of_message = true;

	union {
		struct {
			int task_id;
		} init;

		struct {
			uint8_t key_code;
			uint8_t modifier;
			uint8_t ascii;
			int press;
		} key_input;

		struct {
			timeout_action_t action;
		} timer;

		struct {
			char s[128];
			int level;
		} write;

		struct {
			char buf[256];
		} write_shell;

		struct {
			int status;
		} exit_task;

		struct {
			unsigned int total;
			unsigned int used;
		} memory_usage;

		struct {
			unsigned int device_id;
			unsigned int vendor_id;
			char bus_address[8];
		} pci;

		struct {
			fs_id_t request_id;
			void* buf;
			unsigned int sector;
			size_t len;
			size_t sequence;
			msg_t dst_type;
		} blk_io;

		struct {
			fs_id_t request_id;
			fd_t fd;
			void* buf;
			char name[30];
			size_t len;
			int operation;
			int result;
		} fs_op;
	} data;
};
