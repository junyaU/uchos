#pragma once

#include "../types.hpp"
#include <cstdint>

// flags for sys_ipc
constexpr int IPC_RECV = 0;
constexpr int IPC_SEND = 1;

constexpr int NO_TASK = -1;
constexpr int NOTIFY_KEY_INPUT = 0;
constexpr int NOTIFY_XHCI = 1;
constexpr int NOTIFY_CURSOR_BLINK = 2;
constexpr int NOTIFY_TIMER_TIMEOUT = 3;
constexpr int NOTIFY_WRITE = 4;

constexpr int NUM_MESSAGE_TYPES = 5;

enum class timeout_action_t : uint8_t {
	TERMINAL_CURSOR_BLINK,
	SWITCH_TASK,
};

struct message {
	int32_t type;
	task_t sender;

	union {
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
			const char* s;
			int level;
		} write;
	} data;
};

[[gnu::no_caller_saved_registers]] error_t
send_message(task_t dst, const message* m);
