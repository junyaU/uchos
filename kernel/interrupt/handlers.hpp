/*
 * @file interrupt/handler.hpp
 *
 * @brief interrupt handler
 *
 *
 */

#pragma once

#include <cstdint>

#include "../graphics/log.hpp"

struct InterruptFrame {
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

void on_xhci_interrupt(InterruptFrame* frame);

[[gnu::no_caller_saved_registers]] void kill_userland(InterruptFrame* frame);

#define FaultHandlerWithError(error_code)                                           \
	inline __attribute__((interrupt)) void InterruptHandler##error_code(            \
			InterruptFrame* frame, uint64_t code)                                   \
	{                                                                               \
		kill_userland(frame);                                                       \
		printk(KERN_ERROR, reinterpret_cast<const char*>(#error_code));             \
		printk(KERN_ERROR, "\n");                                                   \
		printk(KERN_ERROR, "RIP : ");                                               \
		printk(KERN_ERROR, "0x%016lx\n", frame->rip);                               \
		printk(KERN_ERROR, "RSP : ");                                               \
		printk(KERN_ERROR, "0x%016lx\n", frame->rsp);                               \
		printk(KERN_ERROR, "RFLAGS : ");                                            \
		printk(KERN_ERROR, "0x%016lx\n", frame->rflags);                            \
		printk(KERN_ERROR, "CS : ");                                                \
		printk(KERN_ERROR, "0x%016lx\n", frame->cs);                                \
		printk(KERN_ERROR, "SS : ");                                                \
		printk(KERN_ERROR, "0x%016lx\n", frame->ss);                                \
		while (true)                                                                \
			__asm__("hlt");                                                         \
	}

#define FaultHandlerNoError(error_code)                                             \
	inline __attribute__((interrupt)) void InterruptHandler##error_code(            \
			InterruptFrame* frame, uint64_t code)                                   \
	{                                                                               \
		kill_userland(frame);                                                       \
		printk(KERN_ERROR, reinterpret_cast<const char*>(#error_code));             \
		while (1)                                                                   \
			__asm__("hlt");                                                         \
	}

FaultHandlerNoError(DE) FaultHandlerNoError(DB) FaultHandlerNoError(
		BP) FaultHandlerNoError(OF) FaultHandlerNoError(BR) FaultHandlerNoError(UD)
		FaultHandlerNoError(NM) FaultHandlerWithError(DF) FaultHandlerWithError(TS)
				FaultHandlerWithError(NP) FaultHandlerWithError(SS)
						FaultHandlerWithError(GP) FaultHandlerWithError(PF)
								FaultHandlerNoError(MF) FaultHandlerWithError(AC)
										FaultHandlerNoError(MC)
												FaultHandlerNoError(XM)
														FaultHandlerNoError(VE)