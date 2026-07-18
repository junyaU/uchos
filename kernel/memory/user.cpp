#include "user.hpp"
#include <cstddef>
#include <cstdint>
#include "graphics/log.hpp"

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
		LOG_ERROR("null pointer in copy_from_user");
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

ssize_t copy_string_from_user(char* to, const char __user* from, size_t max_len)
{
	if (to == nullptr || from == nullptr || max_len == 0) {
		return -1;
	}

	const char* src = static_cast<const char*>(from);
	for (size_t i = 0; i < max_len; ++i) {
		if (!is_user_address(&src[i], 1)) {
			LOG_ERROR("invalid address for copy_string_from_user: %p", &src[i]);
			to[0] = '\0';
			return -1;
		}

		to[i] = src[i];
		if (src[i] == '\0') {
			return static_cast<ssize_t>(i);
		}
	}

	// No terminator within max_len: the string does not fit
	to[max_len - 1] = '\0';
	return -1;
}
