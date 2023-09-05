#include "handler.hpp"
#include "graphics/system_logger.hpp"
#include "task/task_manager.hpp"
#include "timers/timer.hpp"

namespace
{
void NotifyEndOfInterrupt()
{
	volatile auto eoi = reinterpret_cast<uint32_t*>(0xfee000b0);
	*eoi = 0;
}
} // namespace

__attribute__((interrupt)) void TimerInterrupt(InterruptFrame* frame)
{
	const bool need_context_switch = timer->IncrementTick();
	NotifyEndOfInterrupt();

	if (need_context_switch) {
		SwitchTask();
	}
}
