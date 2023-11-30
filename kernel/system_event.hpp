#pragma once

#include <cstdint>

struct SystemEvent {
	enum Type {
		kEmpty,
		kTimerTimeout,
		kDrawScreenTimer,
		kSwitchTask,
		XHCI,
		KEY_PUSH,
	} type_;

	union {
		struct {
			uint64_t id;
			uint64_t timeout;
			unsigned int period;
			int periodical;
		} timer;

		struct {
			uint64_t value;
		} draw_screen_timer;

		struct {
			uint8_t modifier;
			uint8_t keycode;
			uint8_t ascii;
			int press;
		} keyboard;
	} args_;
};