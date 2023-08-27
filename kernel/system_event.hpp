#pragma once

#include <cstdint>

struct SystemEvent {
	enum Type {
		kEmpty,
		kTimerTimeout,
	} type_;

	union {
		struct {
			uint64_t timeout;
		} timer;
	} args_;
};