/*
 * @file system_event.hpp
 *
 * @brief System Event Definitions
 *
 * This file defines the `system_event` structure, which is used throughout the
 * system to handle various types of events such as timers, keyboard inputs, and USB
 * controller events. Each event type is categorized and its data encapsulated in a
 * union, allowing easy access and manipulation of event-specific data.
 *
 */

#pragma once

#include <cstdint>

enum class action_type : uint8_t {
	TERMINAL_CURSOR_BLINK,
};

struct system_event {
	enum type : uint8_t {
		EMPTY,
		TIMER_TIMEOUT,
		SWITCH_TASK,
	} type_;

	union {
		struct {
			uint64_t id;
			uint64_t timeout;
			unsigned int period;
			int periodical;
			action_type action;
		} timer;
	} args_;
};