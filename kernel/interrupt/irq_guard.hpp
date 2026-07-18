/**
 * @file interrupt/irq_guard.hpp
 * @brief Scoped interrupt disabling for short critical sections
 */

#pragma once

#include <cstdint>

namespace kernel::interrupt
{

/**
 * @brief Disable interrupts for a scope, restoring the previous IF state
 *
 * Protects data shared with interrupt handlers (message queues, the run
 * queue) on this single-processor kernel. Safe to nest and safe to use in
 * interrupt context: the destructor re-enables interrupts only if they
 * were enabled on entry.
 */
class IrqGuard
{
public:
	IrqGuard()
	{
		uint64_t rflags;
		asm volatile("pushfq\n\tpopq %0\n\tcli" : "=r"(rflags) : : "memory");
		were_enabled_ = (rflags & 0x200) != 0;
	}

	~IrqGuard()
	{
		if (were_enabled_) {
			asm volatile("sti" : : : "memory");
		}
	}

	IrqGuard(const IrqGuard&) = delete;
	IrqGuard& operator=(const IrqGuard&) = delete;

private:
	bool were_enabled_; ///< IF state on entry
};

} // namespace kernel::interrupt
