/**
 * @file interrupt/routing.cpp
 * @brief IRQ-to-task notification routing table implementation
 */

#include "interrupt/routing.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "error.hpp"
#include "log/log.hpp"
#include "task/ipc.hpp"

namespace
{

struct IrqRoute {
	ProcessId dst;
	kernel::task::NotifyType type;
	bool registered;
};

constexpr size_t NUM_VECTORS = 256;

std::array<IrqRoute, NUM_VECTORS> routes;

uint64_t unrouted_count = 0;

} // namespace

namespace kernel::interrupt
{

error_t register_irq_notification(InterruptVector::Number vector,
								  ProcessId dst,
								  kernel::task::NotifyType type)
{
	const auto index = static_cast<size_t>(vector);
	if (index >= NUM_VECTORS) {
		LOG_ERROR_CODE(ERR_INVALID_ARG, "invalid interrupt vector: %d",
					   static_cast<int>(vector));
		return ERR_INVALID_ARG;
	}

	routes[index] = IrqRoute{ dst, type, true };

	return OK;
}

[[gnu::no_caller_saved_registers]] void notify_irq(InterruptVector::Number vector)
{
	const auto index = static_cast<size_t>(vector);
	if (index >= NUM_VECTORS || !routes[index].registered) {
		++unrouted_count;
		return;
	}

	kernel::task::notify(routes[index].dst, routes[index].type);
}

uint64_t unrouted_irq_count() { return unrouted_count; }

} // namespace kernel::interrupt
