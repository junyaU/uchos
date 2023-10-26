#pragma once

#include "../system_event.hpp"
#include <cstdint>
#include <memory>
#include <queue>
#include <unordered_set>

inline bool operator<(const SystemEvent& lhs, const SystemEvent& rhs)
{
	return lhs.args_.timer.timeout > rhs.args_.timer.timeout;
}

static const int kTimerFrequency = 100;
static const int kSwitchTextMillisec = 20;

class Timer
{
public:
	Timer() : tick_{ 0 }, last_id_{ 1 } {}

	uint64_t AddTimerEvent(unsigned long millisec);

	uint64_t AddPeriodicTimerEvent(unsigned long millisec, uint64_t id = 0);

	void AddSwitchTaskEvent(unsigned long millisec);

	void RemoveTimerEvent(uint64_t id);

	bool IncrementTick();

private:
	uint64_t CalculateTimeoutTicks(unsigned long millisec) const;

	uint64_t tick_;
	uint64_t last_id_;
	std::unordered_set<uint64_t> ignore_events_;
	std::priority_queue<SystemEvent> events_;
};

extern Timer* timer;

void InitializeTimer();
