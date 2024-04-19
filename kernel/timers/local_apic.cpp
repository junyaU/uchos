#include "local_apic.hpp"
#include "graphics/log.hpp"
#include "interrupt/vector.hpp"
#include "timers/acpi.hpp"
#include "timers/timer.hpp"
#include <libs/common/types.hpp>

namespace local_apic
{
// register for setting operating mode and interrupts
volatile uint32_t& LVT_TIMER = *reinterpret_cast<uint32_t*>(0xfee00320);
// register to set the initial value of the counter
volatile uint32_t& INITIAL_COUNT = *reinterpret_cast<uint32_t*>(0xfee00380);
// register to read the current value of the counter
volatile uint32_t& CURRENT_COUNT = *reinterpret_cast<uint32_t*>(0xfee00390);
// register to set the divide value of the counter
volatile uint32_t& DIVIDE_CONF = *reinterpret_cast<uint32_t*>(0xfee003e0);

constexpr uint32_t COUNT_MAX = 0xffffffffU;

void initialize()
{
	printk(KERN_INFO, "Initializing local APIC...");

	DIVIDE_CONF = 0b1011;	   // divide by 1
	LVT_TIMER = (0b001 << 16); // mask

	INITIAL_COUNT = COUNT_MAX;

	acpi::wait_by_pm_timer(100);
	const uint32_t elapsed = COUNT_MAX - CURRENT_COUNT;

	INITIAL_COUNT = 0;

	const uint32_t freq = elapsed * 10;

	printk(KERN_INFO, "Local APIC timer frequency: %u Hz", freq);

	DIVIDE_CONF = 0b1011; // divide by 1

	LVT_TIMER = (0b010 << 16) | interrupt_vector::LOCAL_APIC_TIMER;

	INITIAL_COUNT = freq / TIMER_FREQUENCY;

	printk(KERN_INFO, "Local APIC initialized successfully.");
}

} // namespace local_apic