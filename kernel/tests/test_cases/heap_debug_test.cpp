#include "tests/test_cases/heap_debug_test.hpp"

#ifdef KERNEL_HEAP_DEBUG_ENABLED

#include <cstdint>
#include "memory/heap_debug.hpp"
#include "memory/slab.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

namespace
{

namespace hd = kernel::memory::heap_debug;

// A size class large enough that a slab holds exactly one object, so a freed
// object is deterministically handed back on the next allocation of that size.
// 200000 + redzones rounds up to a 256 KiB object, and 256 KiB / 256 KiB = 1.
constexpr size_t SINGLE_OBJECT_SIZE = 200000;

// A live allocation must be tracked and must disappear again once freed. This
// is the primitive the per-suite leak check is built on: an unfreed allocation
// stays counted.
void test_heap_debug_tracks_live()
{
	const size_t before = hd::live_count();

	void* p = kernel::memory::alloc(64, kernel::memory::ALLOC_UNINITIALIZED);
	ASSERT_NOT_NULL(p);
	ASSERT_EQ(hd::live_count(), before + 1);
	ASSERT_TRUE(kernel::memory::is_slab_object_in_use(p));

	kernel::memory::free(p);
	ASSERT_EQ(hd::live_count(), before);
	// Deliberately probes a freed pointer; the analyzer cannot know this read
	// is safe by construction (see ExpectedViolation usages below).
	// NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
	ASSERT_FALSE(kernel::memory::is_slab_object_in_use(p));
}

// Freeing the same pointer twice must be caught at the second free.
void test_heap_debug_detects_double_free()
{
	const size_t before = hd::stats().double_free;

	void* p = kernel::memory::alloc(64, kernel::memory::ALLOC_UNINITIALIZED);
	ASSERT_NOT_NULL(p);

	kernel::memory::free(p);
	{
		const hd::ExpectedViolation expected;
		kernel::memory::free(p); // double free NOLINT(clang-analyzer-unix.Malloc)
	}

	ASSERT_EQ(hd::stats().double_free, before + 1);
}

// Writing to a freed object corrupts its poison; the corruption must be caught
// when the object is next handed out.
void test_heap_debug_detects_use_after_free()
{
	const size_t before = hd::stats().use_after_free;

	void* p = kernel::memory::alloc(SINGLE_OBJECT_SIZE,
									kernel::memory::ALLOC_UNINITIALIZED);
	ASSERT_NOT_NULL(p);
	kernel::memory::free(p);

	// Use-after-free: scribble on the freed payload (volatile so the store is
	// not optimised away).
	// NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
	static_cast<volatile uint8_t*>(p)[0] = 0x12;

	// The same object comes back and its broken poison is detected.
	void* q;
	{
		const hd::ExpectedViolation expected;
		q = kernel::memory::alloc(SINGLE_OBJECT_SIZE,
								  kernel::memory::ALLOC_UNINITIALIZED);
	}
	ASSERT_EQ(q, p);
	ASSERT_EQ(hd::stats().use_after_free, before + 1);

	kernel::memory::free(q);
}

// Writing one byte past the end of the payload corrupts the tail redzone and
// must be caught at free().
void test_heap_debug_detects_overflow()
{
	const size_t before = hd::stats().redzone;

	constexpr size_t size = 64;
	auto* p = static_cast<volatile uint8_t*>(
			kernel::memory::alloc(size, kernel::memory::ALLOC_UNINITIALIZED));
	ASSERT_NOT_NULL(p);

	p[size] = 0xAA; // one byte past the payload -> tail redzone

	{
		const hd::ExpectedViolation expected;
		kernel::memory::free(const_cast<uint8_t*>(p));
	}
	ASSERT_EQ(hd::stats().redzone, before + 1);
}

} // namespace

void register_heap_debug_tests()
{
	test_register("heap_debug_tracks_live", test_heap_debug_tracks_live);
	test_register("heap_debug_double_free", test_heap_debug_detects_double_free);
	test_register("heap_debug_use_after_free",
				  test_heap_debug_detects_use_after_free);
	test_register("heap_debug_overflow", test_heap_debug_detects_overflow);
}

#else

void register_heap_debug_tests() {}

#endif // KERNEL_HEAP_DEBUG_ENABLED
