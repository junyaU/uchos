/*
 * @file interrupt/handler.hpp
 *
 * @brief interrupt handler
 *
 *
 */

#pragma once

#include <cstdint>

struct interrupt_frame {
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

void on_xhci_interrupt(interrupt_frame* frame);

void on_virtio_interrupt(interrupt_frame* frame);

void on_virtqueue_interrupt(interrupt_frame* frame);

[[gnu::no_caller_saved_registers]] void kill_userland(interrupt_frame* frame);
