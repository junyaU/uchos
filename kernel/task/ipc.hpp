#pragma once

#include "../types.hpp"
#include <cstdint>

#define NOTIFY_KEY_INPUT 1
#define NOTIFY_XHCI 2
#define NOTIFY_CURSOR_BLINK 3

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
	} data;
};

error_t send_message(task_t dst, const message* m);
