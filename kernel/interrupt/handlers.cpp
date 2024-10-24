#include "handlers.hpp"
#include "asm_utils.h"
#include "graphics/log.hpp"
#include "task/context.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"
#include "timers/timer.hpp"
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/types.hpp>

namespace
{
[[gnu::no_caller_saved_registers]] void notify_end_of_interrupt()
{
	auto* volatile eoi = reinterpret_cast<uint32_t*>(0xfee000b0);
	*eoi = 0;
}
} // namespace

__attribute__((interrupt)) void on_xhci_interrupt(interrupt_frame* frame)
{
	message m = { msg_t::NOTIFY_XHCI, INTERRUPT_TASK, {} };
	send_message(XHCI_TASK_ID, &m);
	notify_end_of_interrupt();
}

__attribute__((interrupt)) void on_virtio_interrupt(interrupt_frame* frame)
{
	LOG_ERROR("virtio interrupt");
	notify_end_of_interrupt();
}

__attribute__((interrupt)) void on_virtio_blk_queue_interrupt(interrupt_frame* frame)
{
	schedule_task(VIRTIO_BLK_TASK_ID);
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

extern "C" void switch_task_by_interrupt(context* ctx)
{
	notify_end_of_interrupt();
	switch_task(*ctx);
}

void kill_userland(interrupt_frame* frame)
{
	auto cpl = frame->cs & 0x3;
	if (cpl != 3) {
		return;
	}

	__asm__("sti");

	exit_userland(CURRENT_TASK->kernel_stack_ptr, 128);
}
