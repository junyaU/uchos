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

struct system_event {
	enum Type {
		EMPTY,
		TIMER_TIMEOUT,
		DRAW_SCREEN_TIMER,
		SWITCH_TASK,
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