#include "handler.hpp"
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
	timer->IncrementTick();
	NotifyEndOfInterrupt();
}
