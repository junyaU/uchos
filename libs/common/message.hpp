#pragma once

#include "types.hpp"
#include <cstdint>

// flags for sys_ipc
constexpr int IPC_RECV = 0;
constexpr int IPC_SEND = 1;

// ipc message types
constexpr int NO_TASK = -1;
constexpr int NOTIFY_KEY_INPUT = 0;
constexpr int NOTIFY_XHCI = 1;
constexpr int NOTIFY_TIMER_TIMEOUT = 3;
constexpr int NOTIFY_WRITE = 4;
constexpr int IPC_FILE_SYSTEM_OPERATION = 5;
constexpr int IPC_INITIALIZE_TASK = 6;
constexpr int IPC_TIME = 7;
constexpr int IPC_EXIT_TASK = 8;
constexpr int IPC_MEMORY_USAGE = 9;
constexpr int IPC_PCI = 10;
constexpr int IPC_WRITE_TO_BLK_DEVICE = 12;
constexpr int IPC_READ_FROM_BLK_DEVICE = 13;

// file system operations
constexpr int FS_OP_LIST = 0;
constexpr int FS_OP_READ = 1;

constexpr int NUM_MESSAGE_TYPES = 15;

enum class timeout_action_t : uint8_t {
	TERMINAL_CURSOR_BLINK,
	SWITCH_TASK,
};

struct message {
	int32_t type;
	pid_t sender;
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
			char path[30];
			int operation;
		} fs_operation;

		struct {
			char buf[128];
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
			char buf[100];
			unsigned int sector;
			unsigned int len;
		} blk_device;

	} data;
};
