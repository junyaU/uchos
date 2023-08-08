#pragma once

#include <cstddef>
#include <cstdint>

// Advanced Configuration and Power Interface

namespace acpi {
// Root System Description Pointer
struct RSDP {
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

    bool IsValid() const;
} __attribute__((packed));

struct SDTHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;

    bool IsValid(const char* expected_signature) const;
} __attribute__((packed));

// eXtended System Descripter Table
struct XSDT {
    SDTHeader header;

    const SDTHeader& operator[](size_t i) const {
        const auto* entry_addr = reinterpret_cast<const uint64_t*>(&header + 1);
        return *reinterpret_cast<const SDTHeader*>(entry_addr[i]);
    }

    size_t Count() const {
        return (header.length - sizeof(header)) / sizeof(uint64_t);
    }
} __attribute__((packed));

// Fixed ACPI Description Table
struct FADT {
    SDTHeader header;
    char reserved1[76 - sizeof(header)];
    uint32_t pm_tmr_blk;
    char reserved2[112 - 80];
    uint32_t flags;
    char reserved3[276 - 116];
} __attribute__((packed));

extern const FADT* fadt;

void Initialize(const RSDP& rsdp);

void WaitByPMTimer(unsigned long millisec);
}  // namespace acpi
