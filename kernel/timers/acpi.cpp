#include "acpi.hpp"
#include "asm_utils.h"
#include "graphics/log.hpp"
#include <libs/common/types.hpp>
#include <string.h>

namespace kernel::timers::acpi
{
bool root_system_description_pointer::is_valid() const
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

bool sdt_header::is_valid(const char* expected_signature) const
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

const fixed_acpi_description_table* fadt;

void initialize(const root_system_description_pointer& rsdp)
{
	LOG_INFO("Initializing ACPI...");

	if (!rsdp.is_valid()) {
		LOG_ERROR("acpi is invalid");
		return;
	}

	fadt = nullptr;
	const auto* xsdt = reinterpret_cast<const extended_system_description_table*>(
			rsdp.xsdt_address);
	for (int i = 0; i < xsdt->Count(); i++) {
		const auto& entry = (*xsdt)[i];
		if (entry.is_valid("FACP")) {
			fadt = reinterpret_cast<const fixed_acpi_description_table*>(&entry);
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

	const bool enable32bit = ((fadt->flags >> 8) & 1) != 0;
	if (!enable32bit) {
		end_count &= 0x00ffffffU;
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
	const bool enable32bit = ((fadt->flags >> 8) & 1) != 0;
	if (enable32bit) {
		return count;
	}

	return count & 0x00ffffffU;
}

float pm_timer_count_to_millisec(uint32_t count)
{
	return static_cast<float>(count) * 1000 / PM_TIMER_FREQUENCY;
}

} // namespace kernel::timers::acpi
