#include "user.hpp"
#include "../graphics/log.hpp"
#include <cstddef>
#include <cstdint>

bool is_user_address(const void* addr, size_t n)
{
	const uintptr_t user_start = 0xffff'8000'0000'0000;
	const uintptr_t user_end = 0xffff'ffff'ffff'ffff;
	const uintptr_t addr_val = reinterpret_cast<uintptr_t>(addr);

	return addr_val >= user_start && addr_val < user_end && addr_val + n < user_end;
}

size_t copy_to_user(void __user* to, const void* from, size_t n)
{
	if (to == nullptr || from == nullptr) {
		return 0;
	}

	if (!is_user_address(to, n)) {
		LOG_ERROR("invalid address for copy_to_user: %p", to);
		return 0;
	}

	for (size_t i = 0; i < n; ++i) {
		((uint8_t*)to)[i] = ((uint8_t*)from)[i];
	}

	return n;
}

size_t copy_from_user(void* to, const void __user* from, size_t n)
{
	if (to == nullptr || from == nullptr) {
		return 0;
	}

	if (!is_user_address(from, n)) {
		LOG_ERROR("invalid address for copy_from_user: %p", from);
		return 0;
	}

	for (size_t i = 0; i < n; ++i) {
		((uint8_t*)to)[i] = ((uint8_t*)from)[i];
	}

	return n;
}
