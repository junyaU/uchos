#include "acpi.hpp"
#include <cstdint>
#include <cstring>
#include "asm_utils.h"
#include "log/log.hpp"

namespace kernel::timers::acpi
{
namespace
{
// Bit position in FixedAcpiDescriptionTable::flags that indicates whether the
// ACPI PM timer counter is 32-bit wide (set) or 24-bit wide (clear).
constexpr uint32_t PM_TIMER_32BIT_FLAG_BIT = 8;

// Mask for the counter value when the PM timer is only 24 bits wide.
constexpr uint32_t PM_TIMER_COUNTER_MASK_24BIT = 0x00ffffffU;

bool is_pm_timer_32bit(uint32_t flags)
{
	return ((flags >> PM_TIMER_32BIT_FLAG_BIT) & 1) != 0;
}
} // namespace

bool RootSystemDescriptionPointer::is_valid() const
{
	if (strncmp(signature, "RSD PTR ", 8) != 0) {
		return false;
	}

	if (revision != 2) {
		return false;
	}

	uint8_t sum = 0;
	for (size_t i = 0; i < length; ++i) {
		sum += reinterpret_cast<const uint8_t*>(this)[i];
	}

	return sum == 0;
}

bool SdtHeader::is_valid(const char* expected_signature) const
{
	if (strncmp(signature, expected_signature, 4) != 0) {
		return false;
	}

	uint8_t sum = 0;
	for (size_t i = 0; i < length; ++i) {
		sum += reinterpret_cast<const uint8_t*>(this)[i];
	}

	return sum == 0;
}

const FixedAcpiDescriptionTable* fadt;

void initialize(const RootSystemDescriptionPointer& rsdp)
{
	LOG_INFO("Initializing ACPI...");

	if (!rsdp.is_valid()) {
		LOG_ERROR("acpi is invalid");
		return;
	}

	fadt = nullptr;
	const auto* xsdt = reinterpret_cast<const ExtendedSystemDescriptionTable*>(
			rsdp.xsdt_address);
	for (int i = 0; i < xsdt->count(); i++) {
		const auto& entry = (*xsdt)[i];
		if (entry.is_valid("FACP")) {
			fadt = reinterpret_cast<const FixedAcpiDescriptionTable*>(&entry);
			break;
		}
	}

	if (fadt == nullptr) {
		LOG_ERROR("FADT is not found");
		return;
	}

	LOG_INFO("PM timer is available");

	LOG_INFO("ACPI initialized successfully.");
}

const int PM_TIMER_FREQUENCY = 3579545;

void wait_by_pm_timer(unsigned long millisec)
{
	const uint32_t initial_count = read_from_io_port(fadt->pm_tmr_blk);
	uint32_t end_count = initial_count + (PM_TIMER_FREQUENCY * millisec) / 1000;

	const bool enable32bit = is_pm_timer_32bit(fadt->flags);
	if (!enable32bit) {
		end_count &= PM_TIMER_COUNTER_MASK_24BIT;
	}

	const bool wrapped = end_count < initial_count;
	if (wrapped) {
		while (read_from_io_port(fadt->pm_tmr_blk) >= initial_count) {
		};
	}

	while (read_from_io_port(fadt->pm_tmr_blk) < end_count) {
	}
}

uint32_t get_pm_timer_count()
{
	const uint32_t count = read_from_io_port(fadt->pm_tmr_blk);
	const bool enable32bit = is_pm_timer_32bit(fadt->flags);
	if (enable32bit) {
		return count;
	}

	return count & PM_TIMER_COUNTER_MASK_24BIT;
}

float pm_timer_count_to_millisec(uint32_t count)
{
	return static_cast<float>(count) * 1000 / PM_TIMER_FREQUENCY;
}

} // namespace kernel::timers::acpi
