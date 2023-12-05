#include "bit_utils.hpp"
#include <climits>

unsigned int bit_width(unsigned int x)
{
	if (x == 0) {
		return 0;
	}

	return CHAR_BIT * sizeof(unsigned int) - __builtin_clz(x);
}

unsigned int bit_width_ceil(unsigned int x)
{
	if ((x & (x - 1)) == 0) {
		return bit_width(x) - 1;
	}

	return bit_width(x);
}

unsigned int align_up(unsigned int value, unsigned int alignment)
{
	return (value + alignment - 1) & ~static_cast<unsigned int>(alignment - 1);
}