#include "local_apic.hpp"
#include <cstdint>
#include "graphics/log.hpp"
#include "interrupt/vector.hpp"
#include "timers/acpi.hpp"
#include "timers/timer.hpp"

namespace kernel::timers::local_apic
{
namespace
{
// Local APIC registers are memory-mapped starting at this base address.
constexpr uintptr_t LOCAL_APIC_BASE_ADDRESS = 0xfee00000;
constexpr uintptr_t LVT_TIMER_OFFSET = 0x320;
constexpr uintptr_t INITIAL_COUNT_OFFSET = 0x380;
constexpr uintptr_t CURRENT_COUNT_OFFSET = 0x390;
constexpr uintptr_t DIVIDE_CONFIG_OFFSET = 0x3e0;

// Divide configuration register value that selects "divide by 1".
constexpr uint32_t DIVIDE_BY_1 = 0b1011;
} // namespace

// register for setting operating mode and interrupts
volatile uint32_t& lvt_timer =
		*reinterpret_cast<uint32_t*>(LOCAL_APIC_BASE_ADDRESS + LVT_TIMER_OFFSET);
// register to set the initial value of the counter
volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(
		LOCAL_APIC_BASE_ADDRESS + INITIAL_COUNT_OFFSET);
// register to read the current value of the counter
volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(
		LOCAL_APIC_BASE_ADDRESS + CURRENT_COUNT_OFFSET);
// register to set the divide value of the counter
volatile uint32_t& divide_conf = *reinterpret_cast<uint32_t*>(
		LOCAL_APIC_BASE_ADDRESS + DIVIDE_CONFIG_OFFSET);

constexpr uint32_t COUNT_MAX = 0xffffffffU;

void initialize()
{
	LOG_INFO("Initializing local APIC...");

	divide_conf = DIVIDE_BY_1; // divide by 1
	lvt_timer = (0b001 << 16); // mask

	initial_count = COUNT_MAX;

	kernel::timers::acpi::wait_by_pm_timer(100);
	const uint32_t elapsed = COUNT_MAX - current_count;

	initial_count = 0;

	const uint32_t freq = elapsed * 10;

	LOG_INFO("Local APIC timer frequency: %u Hz", freq);

	divide_conf = DIVIDE_BY_1; // divide by 1

	lvt_timer = (0b010 << 16) | kernel::interrupt::InterruptVector::LOCAL_APIC_TIMER;

	initial_count = freq / kernel::timers::TIMER_FREQUENCY;

	LOG_INFO("Local APIC initialized successfully.");
}

} // namespace kernel::timers::local_apic
