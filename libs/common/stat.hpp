#pragma once
#include <cstddef>
#include <cstdint>

enum class StatType : uint8_t {
	REGULAR_FILE = 1,
	DIRECTORY = 2,
	SYMLINK = 3,
	FIFO = 4,
	SOCKET = 5,
	BLOCK_DEVICE = 6,
	CHAR_DEVICE = 7,
	UNKNOWN = 0
};

struct Stat {
	char name[13]; ///< 8.3 name: up to 12 characters plus null terminator
	size_t size;
	StatType type;
	uint64_t fs_id;
};
