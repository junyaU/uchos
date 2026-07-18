#include "tests/test_cases/bit_utils_test.hpp"
#include "bit_utils.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

void test_bit_width()
{
	ASSERT_EQ(bit_width(0), 0U);
	ASSERT_EQ(bit_width(1), 1U);
	ASSERT_EQ(bit_width(4), 3U);
	ASSERT_EQ(bit_width(5), 3U);
	ASSERT_EQ(bit_width(8), 4U);
}

void test_bit_width_ceil()
{
	// bit_width_ceil(0) used to underflow and return a huge value (issue #313)
	ASSERT_EQ(bit_width_ceil(0), 0U);
	ASSERT_EQ(bit_width_ceil(1), 0U);
	ASSERT_EQ(bit_width_ceil(2), 1U);
	ASSERT_EQ(bit_width_ceil(5), 3U);
	ASSERT_EQ(bit_width_ceil(8), 3U);
	ASSERT_EQ(bit_width_ceil(9), 4U);
}

void register_bit_utils_tests()
{
	test_register("bit_width", test_bit_width);
	test_register("bit_width_ceil", test_bit_width_ceil);
}
