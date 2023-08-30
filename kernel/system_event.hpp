#pragma once

#include <cstdint>

struct SystemEvent {
	enum Type {
		kEmpty,
		kTimerTimeout,
		kTimerPeriodicTimeout,
	} type_;

	union {
		struct {
			uint64_t id;
			uint64_t timeout;
			uint64_t period;
		} timer;
	} args_;
};