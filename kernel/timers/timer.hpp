#pragma once

#include <cstdint>
#include <queue>
#include <unordered_set>

#include "system_event.hpp"

inline bool operator<(const SystemEvent& lhs, const SystemEvent& rhs)
{
	return lhs.args_.timer.timeout > rhs.args_.timer.timeout;
}

static const int kFrequency = 100;

class Timer
{
public:
	Timer() : tick_{ 0 }, last_id_{ 1 } {}

	uint64_t AddTimerEvent(unsigned long millisec, bool periodic = false);

	void RemoveTimerEvent(uint64_t id);

	void IncrementTick();

private:
	uint64_t tick_;
	uint64_t last_id_;
	std::unordered_set<uint64_t> ignore_events_;
	std::priority_queue<SystemEvent> events_;
};

extern Timer* timer;

void InitializeTimer();
