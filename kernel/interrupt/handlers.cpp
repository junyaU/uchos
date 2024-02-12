#include "handlers.hpp"
#include "../task/context.hpp"
#include "../task/ipc.hpp"
#include "../task/task.hpp"
#include "../timers/timer.hpp"
#include "../types.hpp"
#include <cstdint>

namespace
{
[[gnu::no_caller_saved_registers]] void notify_end_of_interrupt()
{
	auto* volatile eoi = reinterpret_cast<uint32_t*>(0xfee000b0);
	*eoi = 0;
}
} // namespace

__attribute__((interrupt)) void on_xhci_interrupt(InterruptFrame* frame)
{
	const int xhci_task_id = 3;
	const message m = { NOTIFY_XHCI, INTERRUPT_TASK_ID, {} };
	send_message(xhci_task_id, &m);

	notify_end_of_interrupt();
}

extern "C" void switch_task_by_timer_interrupt(context* ctx)
{
	const bool need_switch_task = ktimer->increment_tick();
	notify_end_of_interrupt();

	if (need_switch_task) {
		switch_task(*ctx);
	}
}
