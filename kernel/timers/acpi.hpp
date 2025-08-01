#pragma once

#include <cstddef>
#include <cstdint>

// Advanced Configuration and Power Interface
namespace kernel::timers::acpi
{
struct RootSystemDescriptionPointer {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;

	// RSDP version 2.0
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t extended_checksum;
	uint8_t reserved[3];

	bool is_valid() const;
} __attribute__((packed));

struct SdtHeader {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;

	bool is_valid(const char* expected_signature) const;
} __attribute__((packed));

struct ExtendedSystemDescriptionTable {
	SdtHeader header;

	const SdtHeader& operator[](size_t i) const
	{
		const auto* entry_addr = reinterpret_cast<const uint64_t*>(&header + 1);
		return *reinterpret_cast<const SdtHeader*>(entry_addr[i]);
	}

	size_t Count() const
	{
		return (header.length - sizeof(header)) / sizeof(uint64_t);
	}
} __attribute__((packed));

struct FixedAcpiDescriptionTable {
	SdtHeader header;
	char reserved1[76 - sizeof(header)];
	uint32_t pm_tmr_blk;
	char reserved2[112 - 80];
	uint32_t flags;
	char reserved3[276 - 116];
} __attribute__((packed));

extern const FixedAcpiDescriptionTable* fadt;

void initialize(const RootSystemDescriptionPointer& rsdp);

void wait_by_pm_timer(unsigned long millisec);

uint32_t get_pm_timer_count();

float pm_timer_count_to_millisec(uint32_t count);
} // namespace kernel::timers::acpi
