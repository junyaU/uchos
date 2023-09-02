#include "local_apic.hpp"

#include <cstdlib>
#include <cstring>

#include "acpi.hpp"
#include "interrupt/vector.hpp"
#include "timer.hpp"

namespace local_apic
{
// register for setting operating mode and interrupts
volatile uint32_t& kLvtTimer = *reinterpret_cast<uint32_t*>(0xfee00320);
// register to set the initial value of the counter
volatile uint32_t& kInitialCount = *reinterpret_cast<uint32_t*>(0xfee00380);
// register to read the current value of the counter
volatile uint32_t& kCurrentCount = *reinterpret_cast<uint32_t*>(0xfee00390);
// register to set the divide value of the counter
volatile uint32_t& kDivideConf = *reinterpret_cast<uint32_t*>(0xfee003e0);

const uint32_t kCountMax = 0xffffffffu;

void Initialize()
{
	kDivideConf = 0b1011;	   // divide by 1
	kLvtTimer = (0b001 << 16); // mask

	kInitialCount = kCountMax;

	acpi::WaitByPMTimer(100);
	uint32_t elapsed = kCountMax - kCurrentCount;

	kInitialCount = 0;

	uint64_t freq = static_cast<uint64_t>(elapsed * 10);
	kLvtTimer = (0b010 << 16) | InterruptVector::kLocalApicTimer;
	kInitialCount = freq / kTimerFrequency;
}

} // namespace local_apic