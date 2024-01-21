/*
 * @file interrupt/handler.hpp
 *
 * @brief interrupt handler
 *
 *
 */

#pragma once

#include <cstdint>

#include "../graphics/terminal.hpp"

struct InterruptFrame {
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

void on_xhci_interrupt(InterruptFrame* frame);

#define FaultHandlerWithError(error_code)                                           \
	inline __attribute__((interrupt)) void InterruptHandler##error_code(            \
			InterruptFrame* frame, uint64_t code)                                   \
	{                                                                               \
		main_terminal->print(#error_code);                                          \
		main_terminal->print("\n");                                                 \
		main_terminal->print("RIP : ");                                             \
		main_terminal->print_interrupt_hex(frame->rip);                             \
		main_terminal->print("RSP : ");                                             \
		main_terminal->print_interrupt_hex(frame->rsp);                             \
		main_terminal->print("CS : ");                                              \
		main_terminal->print_interrupt_hex(frame->cs);                              \
		main_terminal->print("SS : ");                                              \
		main_terminal->print_interrupt_hex(frame->ss);                              \
		main_terminal->print("RFLAGS : ");                                          \
		main_terminal->print_interrupt_hex(frame->rflags);                          \
		while (true)                                                                \
			__asm__("hlt");                                                         \
	}

#define FaultHandlerNoError(error_code)                                             \
	inline __attribute__((interrupt)) void InterruptHandler##error_code(            \
			InterruptFrame* frame, uint64_t code)                                   \
	{                                                                               \
		main_terminal->print(#error_code);                                          \
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