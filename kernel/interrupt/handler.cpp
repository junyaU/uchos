#include "handler.hpp"
#include "../task/task_manager.hpp"
#include "../timers/timer.hpp"

namespace
{
[[gnu::no_caller_saved_registers]] void NotifyEndOfInterrupt()
{
	auto* volatile eoi = reinterpret_cast<uint32_t*>(0xfee000b0);
	*eoi = 0;
}
} // namespace

__attribute__((interrupt)) void TimerInterrupt(InterruptFrame* frame)
{
	const bool need_switch_task = ktimer->increment_tick();
	NotifyEndOfInterrupt();

	if (need_switch_task) {
		ktask_manager->switch_task();
	}
}
