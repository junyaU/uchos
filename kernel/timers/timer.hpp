#pragma once

#include "../task/ipc.hpp"
#include <cstdint>
#include <queue>
#include <unordered_set>

struct timer_event {
	uint64_t id;
	uint64_t timeout;
	unsigned int period;
	int periodical;
	timeout_action_t action;
};

inline bool operator<(const timer_event& lhs, const timer_event& rhs)
{
	return lhs.timeout > rhs.timeout;
}

static constexpr int TIMER_FREQUENCY = 100;
static constexpr int SWITCH_TASK_MILLISEC = 20;
static constexpr int CURSOR_BLINK_MILLISEC = 500;

class kernel_timer
{
public:
	kernel_timer() : tick_{ 0 }, last_id_{ 1 }
	{
		events_ = std::priority_queue<timer_event>();
		ignore_events_ = std::unordered_set<uint64_t>();
	}

	uint64_t current_tick() const { return tick_; }

	float tick_to_time(uint64_t tick) const
	{
		return static_cast<float>(tick) / TIMER_FREQUENCY;
	}

	uint64_t add_timer_event(unsigned long millisec, timeout_action_t action);

	uint64_t add_periodic_timer_event(unsigned long millisec,
									  timeout_action_t action,
									  uint64_t id = 0);

	void add_switch_task_event(unsigned long millisec);

	void remove_timer_event(uint64_t id);

	bool increment_tick();

private:
	uint64_t calculate_timeout_ticks(unsigned long millisec) const;

	uint64_t tick_;
	uint64_t last_id_;
	std::unordered_set<uint64_t> ignore_events_;
	std::priority_queue<timer_event> events_;
};

extern kernel_timer* ktimer;

void initialize_timer();
