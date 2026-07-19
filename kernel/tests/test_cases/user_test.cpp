#include "tests/test_cases/user_test.hpp"
#include <string.h> // NOLINT(misc-include-cleaner) - for strlcpy
#include <cstdint>
#include <cstring>
#include "memory/paging.hpp"
#include "memory/user.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

namespace
{
// A user-half address (PML4 index >= 256) reserved for these tests
constexpr uint64_t TEST_USER_STR_VADDR = 0xffff'8000'2000'0000;
} // namespace

void test_copy_string_from_user_rejects_invalid_pointers()
{
	char buf[16];

	// Kernel addresses and null pointers are rejected
	const char* kernel_str = "kernel";
	ASSERT_EQ(copy_string_from_user(buf, kernel_str, sizeof(buf)), -1);
	ASSERT_EQ(copy_string_from_user(buf, nullptr, sizeof(buf)), -1);
}

void test_copy_string_from_user_roundtrip()
{
	// Map one user page in the active address space and place a string
	const kernel::memory::vaddr_t addr{ TEST_USER_STR_VADDR };
	ASSERT_EQ(kernel::memory::setup_page_tables(addr, 1, true), OK);

	char* user_buf = reinterpret_cast<char*>(addr.data);
	strlcpy(user_buf, "hello", 6); // NOLINT(misc-include-cleaner)

	char buf[16];
	ASSERT_EQ(copy_string_from_user(buf, user_buf, sizeof(buf)), 5);
	ASSERT_EQ(strcmp(buf, "hello"), 0);

	// A string that does not fit the destination is rejected but the
	// buffer is still NUL-terminated
	ASSERT_EQ(copy_string_from_user(buf, user_buf, 3), -1);
	ASSERT_EQ(buf[2], '\0');
}

void register_user_copy_tests()
{
	test_register("copy_string_from_user_rejects_invalid_pointers",
				  test_copy_string_from_user_rejects_invalid_pointers);
	test_register("copy_string_from_user_roundtrip",
				  test_copy_string_from_user_roundtrip);
}
