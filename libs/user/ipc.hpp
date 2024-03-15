#pragma once

#include <cstdint>

struct message {
	uint32_t type;
	int sender;

	union {
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
			const char* s;
			int level;
		} write;
	} data;
};

// flags for sys_ipc
constexpr int IPC_RECV = 0;
constexpr int IPC_SEND = 1;

constexpr int NO_TASK = -1;
constexpr int NOTIFY_KEY_INPUT = 0;
constexpr int NOTIFY_XHCI = 1;
constexpr int NOTIFY_CURSOR_BLINK = 2;
constexpr int NOTIFY_TIMER_TIMEOUT = 3;

void receive_message(message* msg);