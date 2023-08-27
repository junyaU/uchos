#pragma once

#include <cstdint>
#include <queue>

#include "system_event.hpp"

inline bool operator<(const SystemEvent& lhs, const SystemEvent& rhs)
{
	return lhs.args_.timer.timeout > rhs.args_.timer.timeout;
}

static const int kFrequency = 100;

class Timer
{
public:
	Timer() : tick_(0) {}

	void AddTimerEvent(unsigned long millisec);

	void IncrementTick();

private:
	uint64_t tick_;
	std::priority_queue<SystemEvent> events_;
};

extern Timer* timer;

void InitializeTimer();
