#include "acpi.hpp"

#include <cstdlib>
#include <cstring>

#include "asm_utils.h"
#include "graphics/system_logger.hpp"

namespace acpi
{
bool RSDP::IsValid() const
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

bool SDTHeader::IsValid(const char* expected_signature) const
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

const FADT* fadt;

void Initialize(const RSDP& rsdp)
{
	if (!rsdp.IsValid()) {
		system_logger->Print("acpi is invalid\n");
		return;
	}

	fadt = nullptr;
	const auto* xsdt = reinterpret_cast<const XSDT*>(rsdp.xsdt_address);
	for (int i = 0; i < xsdt->Count(); i++) {
		const auto& entry = (*xsdt)[i];
		if (entry.IsValid("FACP")) {
			fadt = reinterpret_cast<const FADT*>(&entry);
			break;
		}
	}

	if (fadt == nullptr) {
		system_logger->Print("FADT is not found\n");
		return;
	}
}

const int kPMTimerFrequency = 3579545;

void WaitByPMTimer(unsigned long millisec)
{
	const uint32_t initial_count = ReadFromIoPort(fadt->pm_tmr_blk);
	uint32_t end_count = initial_count + (kPMTimerFrequency * millisec) / 1000;

	const bool enable32bit = (fadt->flags >> 8) & 1;
	if (!enable32bit) {
		end_count &= 0x00ffffffu;
	}

	bool wrapped = end_count < initial_count;
	if (wrapped) {
		while (ReadFromIoPort(fadt->pm_tmr_blk) >= initial_count)
			;
	}

	while (ReadFromIoPort(fadt->pm_tmr_blk) < end_count) {
		// asm volatile("pause");
	}
}
} // namespace acpi
