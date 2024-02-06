#include "handlers.hpp"
#include "../system_event.hpp"
#include "../system_event_queue.hpp"
#include "../task/context.hpp"
#include "../task/task.hpp"
#include "../timers/timer.hpp"
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
	const message m = { NOTIFY_XHCI, 0 };
	send_message(get_task_id_by_name("usb_handler"), &m);

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