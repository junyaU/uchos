#pragma once

#include "../types.hpp"
#include <cstdint>

#define NOTIFY_KEY_INPUT 0
#define NOTIFY_XHCI 1
#define NOTIFY_CURSOR_BLINK 2

#define NUM_MESSAGE_TYPES 3

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
