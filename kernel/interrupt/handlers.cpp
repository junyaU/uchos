#include "handlers.hpp"
#include <cstdint>
#include "asm_utils.h"
#include "interrupt/routing.hpp"
#include "interrupt/vector.hpp"
#include "log/log.hpp"
#include "task/context.hpp"
#include "task/task.hpp"
#include "timers/timer.hpp"

namespace
{
// Local APIC End-Of-Interrupt register (base 0xfee00000 + offset 0xb0).
constexpr uint32_t LAPIC_EOI_ADDR = 0xfee000b0;

[[gnu::no_caller_saved_registers]] void notify_end_of_interrupt()
{
	auto* volatile eoi = reinterpret_cast<uint32_t*>(LAPIC_EOI_ADDR);
	*eoi = 0;
}
} // namespace

namespace kernel::interrupt
{

__attribute__((interrupt)) void on_xhci_interrupt(InterruptFrame* frame)
{
	notify_irq(InterruptVector::XHCI);
	notify_end_of_interrupt();
}

__attribute__((interrupt)) void on_virtio_blk_interrupt(InterruptFrame* frame)
{
	LOG_ERROR("virtio block interrupt");
	notify_end_of_interrupt();
}

__attribute__((interrupt)) void on_virtio_blk_queue_interrupt(InterruptFrame* frame)
{
	notify_irq(InterruptVector::VIRTQUEUE_BLK);
	notify_end_of_interrupt();
}

__attribute__((interrupt)) void on_virtio_net_interrupt(InterruptFrame* frame)
{
	LOG_ERROR("virtio net interrupt");
	notify_end_of_interrupt();
}

__attribute__((interrupt)) void on_virtio_net_rx_queue_interrupt(
		InterruptFrame* frame)
{
	notify_irq(InterruptVector::VIRTQUEUE_NET_RX);
	notify_end_of_interrupt();
}

__attribute__((interrupt)) void on_virtio_net_tx_queue_interrupt(
		InterruptFrame* frame)
{
	notify_irq(InterruptVector::VIRTQUEUE_NET_TX);
	notify_end_of_interrupt();
}

} // namespace kernel::interrupt

extern "C" void switch_task_by_timer_interrupt(kernel::task::Context* ctx)
{
	const bool need_switch_task = kernel::timers::ktimer->increment_tick();
	notify_end_of_interrupt();

	if (need_switch_task) {
		kernel::task::switch_task(*ctx);
	}
}

extern "C" void switch_task_by_interrupt(kernel::task::Context* ctx)
{
	notify_end_of_interrupt();
	switch_task(*ctx);
}

namespace kernel::interrupt
{

void kill_userland(InterruptFrame* frame)
{
	auto cpl = frame->cs & 0x3;
	if (cpl != 3) {
		return;
	}

	__asm__("sti");

	exit_userland(kernel::task::CURRENT_TASK->kernel_stack_ptr, 128);
}

} // namespace kernel::interrupt
