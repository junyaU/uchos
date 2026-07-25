#include <libs/common/memory_layout.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <libs/user/console.hpp>

namespace
{
bool fill_and_check(uint8_t* p, size_t n, uint8_t v)
{
	memset(p, v, n);
	for (size_t i = 0; i < n; ++i) {
		if (p[i] != v) {
			return false;
		}
	}
	return true;
}
} // namespace

int main(int argc, char** argv)
{
	// 1. Far beyond the old fixed 4 KiB sbrk array: proves malloc is backed
	// by the real per-process heap now (issue #315)
	constexpr size_t BIG = 64 * 1024;
	auto* big = static_cast<uint8_t*>(malloc(BIG));
	if (big == nullptr || !fill_and_check(big, BIG, 0xa5)) {
		printu("heaptest: FAIL (64 KiB allocation)");
		return 0;
	}

	// 2. Many small blocks with distinct patterns survive together
	constexpr int COUNT = 32;
	constexpr size_t SMALL = 1024;
	uint8_t* blocks[COUNT];
	for (int i = 0; i < COUNT; ++i) {
		blocks[i] = static_cast<uint8_t*>(malloc(SMALL));
		if (blocks[i] == nullptr ||
			!fill_and_check(blocks[i], SMALL, static_cast<uint8_t>(i))) {
			printu("heaptest: FAIL (small allocation %d)", i);
			return 0;
		}
	}
	for (int i = 0; i < COUNT; ++i) {
		if (blocks[i][0] != static_cast<uint8_t>(i)) {
			printu("heaptest: FAIL (block %d overwritten)", i);
			return 0;
		}
		free(blocks[i]);
	}
	free(big);

	// 3. Exhaustion fails cleanly: sbrk reports ENOMEM past USER_HEAP_SIZE
	// and malloc turns that into nullptr instead of corrupting memory
	void* too_big = malloc(USER_HEAP_SIZE);
	if (too_big != nullptr) {
		printu("heaptest: FAIL (oversize allocation did not fail)");
		return 0;
	}

	printu("heaptest: PASS (64KiB + %dx1KiB + clean ENOMEM)", COUNT);

	// The shell treats a nonzero exit status as "command not found", so
	// results are reported above and the exit status stays 0
	return 0;
}
