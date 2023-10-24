#include "bit_utils.hpp"

#include <algorithm>
#include <climits>
#include <optional>
#include <sys/types.h>

unsigned int bit_width(unsigned int x)
{
	if (x == 0) {
		return 0;
	}

	return CHAR_BIT * sizeof(unsigned int) - __builtin_clz(x);
}

unsigned int bit_ceil(unsigned int x)
{
	if ((x & (x - 1)) == 0) {
		return bit_width(x) - 1;
	}

	return bit_width(x);
}