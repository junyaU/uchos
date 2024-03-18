#pragma once

#include <cstdint>

struct message {
	uint32_t type;
	int sender;

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
			uint8_t action;
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
			bool is_end_of_message;
		} write_shell;

	} data;
};

// flags for sys_ipc
constexpr int IPC_RECV = 0;
constexpr int IPC_SEND = 1;

// ipc message types
constexpr int NO_TASK = -1;
constexpr int NOTIFY_KEY_INPUT = 0;
constexpr int NOTIFY_XHCI = 1;
constexpr int NOTIFY_CURSOR_BLINK = 2;
constexpr int NOTIFY_TIMER_TIMEOUT = 3;
constexpr int NOTIFY_WRITE = 4;
constexpr int IPC_FILE_SYSTEM_OPERATION = 5;
constexpr int IPC_INITIALIZE_TASK = 6;

// file system operations
constexpr int FS_OP_LIST = 0;
constexpr int FS_OP_READ = 1;

constexpr int KERNEL_TASK_ID = 0;

void receive_message(message* msg);

void send_message(int task_id, const message* msg);

void initialize_task();
