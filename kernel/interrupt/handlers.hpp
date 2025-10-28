/*
 * @file interrupt/handler.hpp
 *
 * @brief interrupt handler
 *
 *
 */

#pragma once

#include <cstdint>

namespace kernel::interrupt
{

struct InterruptFrame {
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

void on_xhci_interrupt(InterruptFrame* frame);

void on_virtio_blk_interrupt(InterruptFrame* frame);

void on_virtio_blk_queue_interrupt(InterruptFrame* frame);

void on_virtio_net_interrupt(InterruptFrame* frame);

void on_virtio_net_rx_queue_interrupt(InterruptFrame* frame);

void on_virtio_net_tx_queue_interrupt(InterruptFrame* frame);

[[gnu::no_caller_saved_registers]] void kill_userland(InterruptFrame* frame);

} // namespace kernel::interrupt
