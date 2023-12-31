#pragma once

#include "../system_event.hpp"
#include <cstdint>
#include <queue>
#include <unordered_set>

inline bool operator<(const system_event& lhs, const system_event& rhs)
{
	return lhs.args_.timer.timeout > rhs.args_.timer.timeout;
}

static const int TIMER_FREQUENCY = 100;
static const int SWITCH_TEXT_MILLISEC = 20;
static const int CURSOR_BLINK_MILLISEC = 500;

class kernel_timer
{
public:
	kernel_timer() : tick_{ 0 }, last_id_{ 1 } {}

	uint64_t current_tick() const { return tick_; }

	float tick_to_time(uint64_t tick) const
	{
		return static_cast<float>(tick) / TIMER_FREQUENCY;
	}

	uint64_t add_timer_event(unsigned long millisec, action_type type);

	uint64_t add_periodic_timer_event(unsigned long millisec,
									  action_type type,
									  uint64_t id = 0);

	void add_switch_task_event(unsigned long millisec);

	void remove_timer_event(uint64_t id);

	bool increment_tick();

private:
	uint64_t calculate_timeout_ticks(unsigned long millisec) const;

	uint64_t tick_;
	uint64_t last_id_;
	std::unordered_set<uint64_t> ignore_events_;
	std::priority_queue<system_event> events_;
};

extern kernel_timer* ktimer;

void initialize_timer();
