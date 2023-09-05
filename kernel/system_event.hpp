#pragma once

#include <cstdint>

struct SystemEvent {
	enum Type {
		kEmpty,
		kTimerTimeout,
		kDrawScreenTimer,
		kSwitchTask,
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
	} args_;
};