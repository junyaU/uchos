#pragma once

#include "types.hpp"

enum class SystemProcessId : pid_t {
	INVALID = -1,
	KERNEL = 0,
	IDLE = 1,
	XHCI = 2,
	VIRTIO_BLK = 3,
	FS_FAT32 = 4,
	SHELL = 5,
	VIRTIO_NET = 6,
	NET = 7,
	INTERRUPT = 100
};

class ProcessId
{
private:
	pid_t value_;

	explicit constexpr ProcessId(pid_t value) : value_(value) {}

public:
	constexpr ProcessId() : value_(static_cast<pid_t>(SystemProcessId::INVALID)) {}
	constexpr ProcessId(SystemProcessId id) : value_(static_cast<pid_t>(id)) {}

	static constexpr ProcessId from_raw(pid_t pid) { return ProcessId(pid); }

	constexpr pid_t raw() const { return value_; }

	constexpr bool operator==(const ProcessId& other) const
	{
		return value_ == other.value_;
	}

	constexpr bool operator==(SystemProcessId id) const
	{
		return value_ == static_cast<pid_t>(id);
	}

	constexpr bool operator==(pid_t raw_pid) const { return value_ == raw_pid; }

	constexpr bool operator!=(const ProcessId& other) const
	{
		return value_ != other.value_;
	}
};

namespace process_ids
{
inline constexpr ProcessId INVALID{ SystemProcessId::INVALID };
inline constexpr ProcessId KERNEL{ SystemProcessId::KERNEL };
inline constexpr ProcessId IDLE{ SystemProcessId::IDLE };
inline constexpr ProcessId XHCI{ SystemProcessId::XHCI };
inline constexpr ProcessId VIRTIO_BLK{ SystemProcessId::VIRTIO_BLK };
inline constexpr ProcessId FS_FAT32{ SystemProcessId::FS_FAT32 };
inline constexpr ProcessId SHELL{ SystemProcessId::SHELL };
inline constexpr ProcessId VIRTIO_NET{ SystemProcessId::VIRTIO_NET };
inline constexpr ProcessId NET{ SystemProcessId::NET };
inline constexpr ProcessId INTERRUPT{ SystemProcessId::INTERRUPT };
} // namespace process_ids
